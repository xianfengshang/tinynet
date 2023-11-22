// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "log_types.h"
#include <io/file_stream.h>
#include <memory>
#include "base/io_buffer.h"
namespace tinynet {
namespace wal {
class LogCodec {
    enum class DecodeStatus {
        Begin,
        Header,
        Payload,
        Record,
        End,
        Error
    };
  public:
    LogCodec();
    ~LogCodec();
  public:
    void Write(io::FileStreamPtr& stream, const void *data, size_t len);

    bool Read(io::FileStreamPtr& stream, std::string* buffer);

  public:
    bool has_error() { return decode_status_ == DecodeStatus::Error; }

    int bytes_read() { return bytes_read_; }

    int bytes_write() { return bytes_write_; }
  private:
    bool DecodeHeader();
    bool Decode(io::FileStreamPtr& stream, std::string* buffer);
  private:
    LogHeader header_;
    char buffer_[LOG_BLOCK_SIZE];
    iov_t iov_;
    DecodeStatus decode_status_;
    int decode_len_;
    int bytes_read_;
    int bytes_write_;
};
}
}
