#pragma once
#include "net/stream_socket.h"
#include "redis_types.h"
#include "base/io_buffer_stream.h"
#include "base/string_view.h"
namespace redis {

namespace protocol {
const char* FindCrlf(const char* buff, size_t len);
bool ReadInteger(const char* buff, size_t len, int64_t* integer);
bool ReadInteger(tinynet::IBufferStream& stream, size_t len, int64_t* integer);
bool WriteCommand(tinynet::IOBuffer* io_buf, const char* cmd, size_t len);
bool WriteCommand(tinynet::IOBuffer* io_buf, const std::vector<tinynet::string_view>& iovs);
}
}
