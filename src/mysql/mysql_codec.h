// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "mysql_types.h"
#include "net/stream_socket.h"
#include "base/io_buffer_stream.h"
#include "mysql_protocol.h"
namespace mysql {
class MysqlCodec {
  public:
    MysqlCodec();
    ~MysqlCodec();
  public:
    enum class DecodeStatus {
        None,
        Header,
        HeaderContinued,
        Payload,
        PayloadContinued,
        Message,
        Reset
    };
  private:
    bool Decode(tinynet::net::SocketPtr& sock);
    bool DecodeHeader(tinynet::net::SocketPtr& sock);
    bool DecodePayload(tinynet::net::SocketPtr& sock);
    bool DecodeHeaderContinued(tinynet::net::SocketPtr& sock);
    bool DecodePayloadContinued(tinynet::net::SocketPtr& sock);
  public:
    Packet* Read(tinynet::net::SocketPtr& sock);
    void Write(tinynet::net::SocketPtr& sock, Packet* packet);
    void EnableCompression(bool value) { compress_ = value; }
  private:
    int decode_len_;
    DecodeStatus  decode_status_;
    Packet packet_;
    bool compress_;
};
}