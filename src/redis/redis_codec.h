#pragma once
#include "net/socket.h"
#include "redis_types.h"
#include "base/io_buffer_stream.h"
#include "redis_protocol.h"
#include "base/error_code.h"
#include "base/string_view.h"
namespace redis {

class RedisCodec {
  public:
    bool Decode(tinynet::net::SocketPtr& sock, RedisReply* reply);
    RedisReply* Read(tinynet::net::SocketPtr& sock);
    void Write(tinynet::net::SocketPtr& sock, const tinynet::string_view& cmd);
    void Write(tinynet::net::SocketPtr& sock, const std::vector<tinynet::string_view>& iovs);
  private:
    RedisReply reply_;
};
}
