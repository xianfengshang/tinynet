// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <string>
#include "io/file_stream.h"
#include "log_codec.h"
namespace tinynet {
namespace wal {
class LogCodec;

class LogRecorder {
  public:
    LogRecorder(io::FileStreamPtr stream);
  public:
    //Add a record
    void Put(const std::string& record);
    //Retrieve next record
    bool Next(std::string* record);

    bool has_error() { return codec_.has_error(); }

    int bytes_read() { return codec_.bytes_read(); }

    int bytes_write() { return codec_.bytes_write(); }
  private:
    io::FileStreamPtr stream_;
    LogCodec codec_;
};
}
}
