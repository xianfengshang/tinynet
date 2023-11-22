// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <stdio.h>

namespace tinynet {
namespace io {
class FileStream;
typedef std::shared_ptr<FileStream> FileStreamPtr;

class FileStream :
    public std::enable_shared_from_this<FileStream> {
  public:
    FileStream(FILE* stream);
    ~FileStream();
  private:
    FileStream(const FileStream& o) = delete;
    FileStream(FileStream&& o) = delete;
    FileStream& operator=(const FileStream& o) = delete;
    FileStream& operator=(FileStream&& o) = delete;
  public:
    static FileStreamPtr Open(const char* filename, const char* mode);
    static FileStreamPtr OpenReadable(const char* filename);
    static FileStreamPtr OpenWritable(const char* filename);
    static FileStreamPtr OpenAppendable(const char* filename);
  public:
    size_t Read(void* buf, size_t len);
    size_t Write(const void *buf, size_t len);
    int Seek(uint32_t offset);
    int Flush();
    size_t Length();
    void Truncate(int size);
    int get_fd() {
#ifdef _MSC_VER
        return _fileno(stream_);
#else
        return fileno(stream_);
#endif
    }
  private:
    static const int kFileBufferSize = 65536;
  private:
    FILE* stream_;
    char buf_[kFileBufferSize];
};
}
}