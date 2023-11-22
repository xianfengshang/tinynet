// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include "net/socket.h"
#include "zlib.h"
#include "net/http/http_parser.h"
#include "websocket_protocol.h"
#include "base/io_buffer_stream.h"

namespace tinynet {
namespace websocket {

class WebSocketCodec;
typedef std::shared_ptr<WebSocketCodec> WebSocketCodecPtr;

class WebSocketCodec {
  public:
    enum class CodecSide {
        CLIENT,
        SERVER
    };
  public:
    enum class DecodeStatus {
        None,
        Header,
        HeaderContinued,
        Payload,
        Message,
        Reset
    };
  public:
    WebSocketCodec(CodecSide side);
    WebSocketCodec(const WebSocketCodec&) = delete;
    WebSocketCodec(WebSocketCodec&&) = delete;
    WebSocketCodec& operator=(const WebSocketCodec&) = delete;
  private:
    bool Decode(net::SocketPtr& sock);
    bool DecodeHeader(net::SocketPtr& sock);
    bool DecodeHeaderContinued(net::SocketPtr& sock);
    bool DecodePayload(net::SocketPtr& sock);
  public:
    WebSocketMessage* Read(net::SocketPtr& sock);
    void Write(net::SocketPtr& sock, const WebSocketMessage* msg);
    void Write(net::SocketPtr& sock, Opcode opcode, const char* msg, size_t len);
    void set_max_packet_size(int value) { max_packet_size_ = value; }
  private:
    WebSocketPacket		pkt_;
    WebSocketMessage	msg_;
    DecodeStatus		decode_status_;
    size_t				decode_len_;
    CodecSide			codec_side_;
    int					max_packet_size_;
};
}
}
