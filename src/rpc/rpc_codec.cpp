// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "rpc_codec.h"
#include "base/io_buffer_stream.h"
#include "logging/logging.h"
#include "base/crypto.h"
#include "base/error_code.h"
#include "zero_copy_stream.h"
#include "base/runtime_logger.h"
#include "base/coding.h"

namespace tinynet {
namespace rpc {

static void EncodeHeader(char* buf, const PacketHeader& header) {
    int offset = 0;
    EncodeFixed32(buf + offset,header.type);
    offset += sizeof(uint32_t);
    EncodeFixed32(buf + offset, header.len);
    offset += sizeof(uint32_t);
    EncodeFixed64(buf + offset, header.method);
    offset += sizeof(uint64_t);
    EncodeFixed64(buf + offset, header.seq);
}

void RpcCodec::Write(net::SocketPtr& sock, RpcPacket *packet) {
    if (packet->header.len >= MAX_RPC_PACKET_LEN) {
        sock->SetError(ERROR_RPC_MESSAGETOOLONG);
        return;
    }
    size_t packetSize = RPC_PACKET_HEADER_LEN + packet->body.size();
    sock->wbuf()->reserve(sock->wbuf()->size() + packetSize);
    char* p = sock->wbuf()->end();
    EncodeHeader(p, packet->header);
    p += RPC_PACKET_HEADER_LEN;
    std::copy(packet->body.begin(), packet->body.end(), p);
    sock->wbuf()->commit(packetSize);
    sock->Flush();
}

void RpcCodec::Write(net::SocketPtr& sock, PacketType type, uint64_t seq, uint64_t method, const google::protobuf::Message *msg) {
    PacketHeader header;
    header.len = (uint32_t)msg->ByteSize();
    header.type = (uint32_t)type;
    header.seq = seq;
    header.method = method;

    size_t packetSize = RPC_PACKET_HEADER_LEN + header.len;
    sock->wbuf()->reserve(sock->wbuf()->size() + packetSize);
    char* p = sock->wbuf()->end();
    EncodeHeader(p, header);
    p += RPC_PACKET_HEADER_LEN;
    if (!msg->SerializeToArray(p, (int)header.len)) {
        log_runtime_error("SerializeToArray faild, type:%d, seq:%llu, method:%u, msg:%s", type, seq, method, msg->GetTypeName().c_str());
        return;
    }
    sock->wbuf()->commit(packetSize);
    sock->Flush();
}

PacketHeader* RpcCodec::Read(net::SocketPtr& sock) {
    if (sock->rbuf()->size() < RPC_PACKET_HEADER_LEN) {
        return nullptr;
    }
    const char* p = sock->rbuf()->begin();
    header_.type = DecodeFixed32(p);
    if (header_.type != (uint32_t)PacketType::REQUEST &&
            header_.type != (uint32_t)PacketType::RESPONSE) {
        sock->SetError(ERROR_RPC_DECODEERROR);
        return nullptr;
    }
    p += sizeof(uint32_t);
    header_.len = DecodeFixed32(p);
    if (header_.len > MAX_RPC_PACKET_LEN) {
        sock->SetError(ERROR_RPC_MESSAGETOOLONG);
        return nullptr;
    }
    if (sock->rbuf()->size() < (header_.len + RPC_PACKET_HEADER_LEN)) {
        return nullptr;
    }
    p += sizeof(uint32_t);
    header_.method = DecodeFixed64(p);
    p += sizeof(uint64_t);
    header_.seq = DecodeFixed64(p);
    sock->rbuf()->consume(RPC_PACKET_HEADER_LEN);
    return &header_;
}

google::protobuf::Message* RpcCodec::Read(net::SocketPtr& sock, google::protobuf::Message* body, size_t len) {
    if (sock->rbuf()->size() < len) {
        return nullptr;
    }
    const char* p = sock->rbuf()->begin();
    if (!body->ParseFromArray(p, (int)len)) {
        sock->SetError(ERROR_RPC_DECODEERROR);
        return nullptr;
    }
    sock->rbuf()->consume(len);
    return body;
}
}
}
