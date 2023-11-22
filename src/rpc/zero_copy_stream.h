// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "google/protobuf/io/zero_copy_stream.h"
#include "base/io_buffer_stream.h"
namespace tinynet {
namespace rpc {
class ZeroCopyOutputStream :
    public google::protobuf::io::ZeroCopyOutputStream {
  public:
    explicit ZeroCopyOutputStream(IOBuffer* io_buf);
    ~ZeroCopyOutputStream();
  public:
    ZeroCopyOutputStream(const ZeroCopyOutputStream&) = delete;
    ZeroCopyOutputStream(ZeroCopyOutputStream&&) = default;
    ZeroCopyOutputStream& operator =(const ZeroCopyOutputStream&) = delete;
  public:
    // @ZeroCopyOutputStream
    virtual bool Next(void** data, int* size) override;
    virtual void BackUp(int count) override;
    virtual google::protobuf::int64 ByteCount() const override;
  public:
    void Reset();
    void Commit();
  private:
    IOBuffer* io_buf_;
    size_t position_;
    size_t block_size_;
};

class ZeroCopyInputStream :
    public google::protobuf::io::ZeroCopyInputStream {
  public:
    explicit ZeroCopyInputStream(IOBuffer* io_buf);
    ~ZeroCopyInputStream();
  public:
    ZeroCopyInputStream(const ZeroCopyInputStream&) = delete;
    ZeroCopyInputStream(ZeroCopyInputStream&&) = default;
    ZeroCopyInputStream& operator =(const ZeroCopyInputStream&) = delete;
  public:
    // @ZeroCopyInputStream
    virtual bool Next(const void** data, int* size) override;
    virtual void BackUp(int count) override;
    virtual bool Skip(int count) override;
    virtual google::protobuf::int64 ByteCount() const override;
  public:
    void Reset();
    void Commit();
  private:
    IOBuffer* io_buf_;
    size_t position_;
};
}
}