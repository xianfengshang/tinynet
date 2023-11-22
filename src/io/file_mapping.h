// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
#include <memory>
namespace tinynet {
namespace io {
class FileMappingImpl {
  public:
    FileMappingImpl(): data_(NULL), len_(0) {}
    virtual ~FileMappingImpl() = default;
  public:
    size_t length() const { return len_; }
    const char* data() const { return data_; }
    bool good() const { return data_ != NULL; };
  protected:
    char* data_;
    size_t len_;
};

class FileMapping {
  public:
    FileMapping(int fd);
    ~FileMapping();
  private:
    FileMapping(const FileMapping&) = delete;
    FileMapping& operator=(const FileMapping&) = delete;
  public:
    size_t length() const;
    const char* data() const;
    bool good() const;
  public:
    std::unique_ptr<FileMappingImpl> impl_;
};
}
}