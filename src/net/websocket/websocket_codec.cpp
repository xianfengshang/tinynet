// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "websocket_codec.h"
#include "net/http/http_parser.h"
#include "logging/logging.h"
#include "util/string_utils.h"
#include "util/random_utils.h"
#include "base/error_code.h"
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <endian.h>
#define ntohll be64toh
#define htonll htobe64
#endif

namespace tinynet {
namespace websocket {

static const int kMaxDecodeStep = (int)WebSocketCodec::DecodeStatus::Reset;

WebSocketCodec::WebSocketCodec(CodecSide side):
    decode_status_(DecodeStatus::None),
    decode_len_(0),
    codec_side_(side),
    max_packet_size_(0) {
}

bool WebSocketCodec::DecodeHeader(net::SocketPtr& sock) {
    if (decode_status_ != DecodeStatus::Header) {
        return false;
    }
    char* p = sock->rbuf()->begin();
    int decode_len = 0;
    char buf = *p;
    //first byte
    p++;
    pkt_.fin = buf >> 7;
    pkt_.opcode = buf & 0x0F;
    if (pkt_.opcode != (uint8_t)Opcode::Continue) {
        msg_.opcode = static_cast<Opcode>(pkt_.opcode);
    }
    //second byte
    buf = *p;
    p++;
    pkt_.mask = buf >> 7;
    if (pkt_.mask) {
        decode_len += 4;
    }
    pkt_.payload_length = buf & 0x7F;

    if (pkt_.payload_length == 126) {
        decode_len += 2;
    } else if (pkt_.payload_length == 127) {
        decode_len += 8;
    }
    if (decode_len == 0) {
        decode_status_ = DecodeStatus::Payload;
        decode_len_ = static_cast<size_t>(pkt_.payload_length);
    } else {
        decode_status_ = DecodeStatus::HeaderContinued;
        decode_len_ = decode_len;
    }
    sock->rbuf()->consume(2);
    return true;
}

bool WebSocketCodec::DecodeHeaderContinued(net::SocketPtr& sock) {
    if (decode_status_ != DecodeStatus::HeaderContinued) {
        return false;
    }
    char* p = sock->rbuf()->begin();
    size_t nlen = 0;
    if (pkt_.payload_length == 126) {
        auto plen = (uint16_t*)p;
        pkt_.payload_length = ntohs(*plen);
        nlen += sizeof(uint16_t);
        p+= sizeof(uint16_t);
    } else if (pkt_.payload_length == 127) {
        auto plen = (uint64_t*)p;
        pkt_.payload_length = ntohll(*plen);
        nlen += sizeof(uint64_t);
        p += sizeof(uint64_t);
    }
    if (pkt_.mask) {
        for (size_t i = 0; i < 4; ++i) {
            pkt_.masking_key[i] = p[i];
        }
        p += 4;
        nlen += 4;
    }
    sock->rbuf()->consume(nlen);
    decode_status_ = DecodeStatus::Payload;
    decode_len_ = static_cast<size_t>(pkt_.payload_length);
    if (max_packet_size_ > 0 && (int)decode_len_ > max_packet_size_) {
        sock->SetError(ERROR_WEBSOCKET_PAYLOAD_LENGTH);
        return false;
    }
    return true;
}

bool WebSocketCodec::DecodePayload(net::SocketPtr& sock) {
    if (decode_len_ > 0) {
        size_t pos = msg_.data.size();
        msg_.data.resize(pos + decode_len_);
        char* data = &msg_.data[pos];
        sock->rbuf()->read(data, decode_len_);
        if (pkt_.mask) {
            for (size_t i = 0; i < decode_len_; ++i) {
                data[i] = data[i] ^ pkt_.masking_key[i & 0x3];
            }
        }
    }
    if (pkt_.fin) {
        decode_status_ = DecodeStatus::Message;
        decode_len_ = 0;
    } else {
        decode_status_ = DecodeStatus::Header;
        decode_len_ = 2;
    }
    return true;
}

bool WebSocketCodec::Decode(net::SocketPtr& sock) {
    if (sock->rbuf()->size() < decode_len_) {
        return false;
    }
    IOBufferStream stream(sock->rbuf());
    switch (decode_status_) {
    case DecodeStatus::None: {
        decode_status_ = DecodeStatus::Header;
        decode_len_ = 2;
        return true;
    }
    case DecodeStatus::Header: {
        return DecodeHeader(sock);
    }
    case DecodeStatus::HeaderContinued: {
        return DecodeHeaderContinued(sock);
    }
    case DecodeStatus::Payload: {
        return DecodePayload(sock);
    }
    case DecodeStatus::Message: {
        return false;
    }
    case DecodeStatus::Reset: {
        std::string data;
        msg_.data.swap(data);
        decode_status_ = DecodeStatus::Header;
        decode_len_ = 2;
        return true;
    }
    default:
        break;
    }
    return false;
}

WebSocketMessage* WebSocketCodec::Read(net::SocketPtr& sock) {
    while (Decode(sock));

    if (decode_status_ == DecodeStatus::Message) {
        decode_status_ = DecodeStatus::Reset;
        decode_len_ = 0;
        return &msg_;
    }
    return nullptr;
}

void WebSocketCodec::Write(net::SocketPtr& sock, const WebSocketMessage *msg ) {
    if (msg->data_ref != nullptr) {
        Write(sock, msg->opcode, msg->data_ref, msg->data_len);
    } else if (msg->bytes_ref != nullptr) {
        Write(sock, msg->opcode, msg->bytes_ref->data(), msg->bytes_ref->size());
    } else {
        Write(sock, msg->opcode, msg->data.data(), msg->data.size());
    }
}

void WebSocketCodec::Write(net::SocketPtr& sock, Opcode opcode, const char* msg, size_t len) {
    sock->wbuf()->reserve(sock->wbuf()->size() + MAX_FRAME_HEADER_LENGTH + len);

    char* buf = sock->wbuf()->end();
    char* p = buf;
    size_t nlen = 0;
    *p = (char)opcode | 0x80;
    p++;
    nlen++;
    uint8_t payloadlen = 0;
    if (len < 126U) {
        payloadlen = static_cast<uint8_t>(len);
        *p = (char)payloadlen;
        p++;
        nlen++;
    } else if (len <= 0xffff) {
        payloadlen = 126;
        *p = (char)payloadlen;
        p++;
        nlen++;

        auto* plen = (uint16_t*)p;
        *plen = htons(static_cast<uint16_t>(len));
        p += sizeof(uint16_t);
        nlen += sizeof(uint16_t);
    } else {
        payloadlen = 127;
        *p = (char)payloadlen;
        p++;
        nlen++;

        auto* plen = (uint64_t*)p;
        *plen = htonll(static_cast<uint64_t>(len));
        p += sizeof(uint64_t);
        nlen += sizeof(uint64_t);
    }
    if (codec_side_ == CodecSide::CLIENT) {
        buf[1] |= 0x80;
        for (int i = 0; i < 4; ++i) {
            p[i] = (char)RandomUtils::Random8();
        }
        char* mask_key = p;
        p += 4;
        nlen += 4;
        for (size_t i = 0; i < len; ++i) {
            p[i] = msg[i] ^ mask_key[i & 0x03];
        }
    } else {
        for (size_t i = 0; i < len; ++i) {
            p[i] = msg[i];
        }
    }
    p += len;
    nlen += len;
    sock->wbuf()->commit(nlen);
    sock->Flush();
}
}
}
