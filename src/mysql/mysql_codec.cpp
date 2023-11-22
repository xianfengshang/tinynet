// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "mysql_codec.h"
#include "base/coding.h"
#include "base/io_buffer_stream.h"
#include "mysql/mysql_protocol.h"
namespace mysql {

MysqlCodec::MysqlCodec():
    decode_len_(0),
    decode_status_(DecodeStatus::None),
    compress_(false) {
}

MysqlCodec::~MysqlCodec() = default;

bool MysqlCodec::Decode(tinynet::net::SocketPtr& sock) {
    if (sock->rbuf()->size() < (size_t)decode_len_) {
        return false;
    }
    switch (decode_status_) {
    case DecodeStatus::None: {
        decode_status_ = DecodeStatus::Header;
        decode_len_ = 4;
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
    case DecodeStatus::PayloadContinued: {
        return DecodePayloadContinued(sock);
    }
    case DecodeStatus::Message: {
        return false;
    }
    case DecodeStatus::Reset: {
        std::string data;
        packet_.data.swap(data);
        decode_status_ = DecodeStatus::Header;
        decode_len_ = 4;
        return true;
    }
    default:
        break;
    }
    return false;
}

bool MysqlCodec::DecodeHeader(tinynet::net::SocketPtr& sock) {
    if (decode_status_ != DecodeStatus::Header) {
        return false;
    }
    const char* header = sock->rbuf()->begin();
    decode_len_ = tinynet::DecodeFixed24(header);
    packet_.len = decode_len_;
    packet_.seq = static_cast<uint8_t>(header[3]);
    decode_status_ = DecodeStatus::Payload;
    sock->rbuf()->consume(4);
    return true;
}

bool MysqlCodec::DecodePayload(tinynet::net::SocketPtr& sock) {
    if (decode_status_ != DecodeStatus::Payload) {
        return false;
    }
    packet_.data.resize(decode_len_);
    sock->rbuf()->read(&packet_.data[0], decode_len_);
    if (packet_.len > 0) {
        uint8_t type = static_cast<uint8_t>(packet_.data[0]);
        switch (type) {
        case 0x00:
            packet_.type = packet_.len >= 7 ? PACKET_TYPE_OK: PACKET_TYPE_EOF;
            break;
        case 0x01:
            packet_.type = PACKET_TYPE_AUTHMOREDATA;
            break;
        case 0xfb:
            packet_.type = PACKET_TYPE_LOCALINFILE;
            break;
        case 0xfe:
            packet_.type = packet_.len >= 7 ? PACKET_TYPE_OK : PACKET_TYPE_EOF;
            break;
        case 0xff:
            packet_.type = PACKET_TYPE_ERR;
            break;
        default:
            packet_.type = PACKET_TYPE_DATA;
            break;
        }
    }
    if (decode_len_ < (int)MAX_PACKET_LENGTH) {
        decode_status_ = DecodeStatus::Message;
    } else {
        decode_status_ = DecodeStatus::HeaderContinued;
        decode_len_ = 4;
    }
    return true;
}

bool MysqlCodec::DecodeHeaderContinued(tinynet::net::SocketPtr& sock) {
    if (decode_status_ != DecodeStatus::HeaderContinued) {
        return false;
    }
    const char* header = sock->rbuf()->begin();
    decode_len_ = tinynet::DecodeFixed24(header);
    packet_.len += decode_len_;
    packet_.seq = static_cast<uint8_t>(header[3]);
    decode_status_ = DecodeStatus::PayloadContinued;
    sock->rbuf()->consume(4);
    return true;
}

bool MysqlCodec::DecodePayloadContinued(tinynet::net::SocketPtr& sock) {
    if (decode_status_ != DecodeStatus::PayloadContinued) {
        return false;
    }
    size_t pos = packet_.data.size();
    packet_.data.resize(pos + decode_len_);
    sock->rbuf()->read(&packet_.data[pos], decode_len_);

    if (decode_len_ < (int)MAX_PACKET_LENGTH) {
        decode_status_ = DecodeStatus::Message;
    } else {
        decode_status_ = DecodeStatus::HeaderContinued;
        decode_len_ = 4;
    }
    return true;
}

Packet* MysqlCodec::Read(tinynet::net::SocketPtr& sock) {
    while (Decode(sock)) {
    }
    if (decode_status_ == DecodeStatus::Message) {
        decode_status_ = DecodeStatus::Reset;
        decode_len_ = 0;
        return &packet_;
    }
    return nullptr;
}

void MysqlCodec::Write(tinynet::net::SocketPtr& sock, Packet* packet) {
    size_t packetSize = packet->len + 4;
    sock->wbuf()->reserve(sock->wbuf()->size() + packetSize);
    char* first = sock->wbuf()->end();
    char* p = first;
    tinynet::EncodeFixed24(p, packet->len);
    p[3] = packet->seq & 0xff;
    p += 4;
    memcpy(p, &packet->data[0], packet->len);
    p += packet->len;
    sock->wbuf()->commit(p - first);
    sock->Flush();
}

}