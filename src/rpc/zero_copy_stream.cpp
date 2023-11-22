// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "zero_copy_stream.h"
namespace tinynet {
namespace rpc {
static const int kDefaultBlockSize = 8192;

ZeroCopyOutputStream::ZeroCopyOutputStream(IOBuffer* io_buf) :
    io_buf_(io_buf),
    position_(0),
    block_size_(kDefaultBlockSize) {
    io_buf_->reserve(io_buf_->size() + block_size_);
}

ZeroCopyOutputStream::~ZeroCopyOutputStream() {
}

bool ZeroCopyOutputStream::Next(void** data, int* size) {
    if (position_ >= block_size_) {
        block_size_ += block_size_;
        io_buf_->reserve(io_buf_->size() + block_size_);
    }
    *data = io_buf_->end() + position_;
    *size = static_cast<int>(block_size_ - position_);
    position_ = block_size_;
    return true;
}

void ZeroCopyOutputStream::BackUp(int count) {
    position_ -= static_cast<size_t>(count);
}

google::protobuf::int64 ZeroCopyOutputStream::ByteCount() const {
    return position_;
}

void ZeroCopyOutputStream::Reset() {
    position_ = 0;
}

void ZeroCopyOutputStream::Commit() {
    io_buf_->commit(position_);
}

ZeroCopyInputStream::ZeroCopyInputStream(IOBuffer* io_buf) :
    io_buf_(io_buf),
    position_(0) {
}

ZeroCopyInputStream::~ZeroCopyInputStream() {
}

bool ZeroCopyInputStream::Next(const void** data, int* size) {
    if (position_ >= io_buf_->size()) {
        return false;
    }
    *data = io_buf_->begin() + position_;
    *size = static_cast<int>(io_buf_->size() - position_);
    position_ = io_buf_->size();
    return true;
}

void ZeroCopyInputStream::BackUp(int count) {
    position_ -= static_cast<size_t>(count);
}

bool ZeroCopyInputStream::Skip(int count) {
    if (position_ + count > io_buf_->size()) {
        position_ = io_buf_->size();
        return false;
    }
    position_ += static_cast<size_t>(count);
    return true;
}
google::protobuf::int64 ZeroCopyInputStream::ByteCount() const {
    return position_;
}

void ZeroCopyInputStream::Reset() {
    position_ = 0;
}

void ZeroCopyInputStream::Commit() {
    io_buf_->commit(position_);
}
}
}
