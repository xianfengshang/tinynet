// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include <limits>
#include <cstring>
#include <cstdio>
#include "base.h"

namespace tinynet {

#ifdef _WIN32
struct iov_t {
    unsigned long len;
    char* base;
};
#else
struct iov_t {
    char* base;
    size_t len;
};
#endif
typedef std::vector<iov_t> iovs_t;

class IOBuffer {
  public:
    static const size_t MIN_BLOCK_SIZE = 32;
    static const size_t npos = static_cast<size_t>(-1);
  public:
    IOBuffer() = default;
    ~IOBuffer() = default;
  private:
    IOBuffer(const IOBuffer&);
    void operator=(const IOBuffer&);
  public:
    TINYNET_FORCEINLINE size_t append(const void *data, size_t len) noexcept {
        std::memcpy(prepare(len), data, len);
        commit(len);
        return len;
    }

    TINYNET_FORCEINLINE size_t append(size_t count, char ch) noexcept {
        std::memset(prepare(count), ch, count);
        commit(count);
        return count;
    }

    TINYNET_FORCEINLINE char pop_front() noexcept {
        char ch = '\0';
        if (!empty()) {
            ch = *begin();
            consume(1);
        }
        return ch;
    }

    TINYNET_FORCEINLINE void push_back(char ch) noexcept {
        append(1, ch);
    }

    size_t copy(void *buf, size_t len, size_t pos = 0) noexcept {
        size_t sz = size();
        if (pos < sz) {
            len = (std::min)(sz - pos, len);
            std::memcpy(buf, begin() + pos, len);
            return len;
        }
        return 0;
    }

    size_t read(void* out_buf, size_t out_len) noexcept {
        size_t sz = size();
        if (sz >= out_len) {
            std::memcpy(out_buf, begin(), out_len);
            consume(out_len);
            return out_len;
        }
        return 0;
    }

    TINYNET_FORCEINLINE size_t put(const void * buf, size_t len, size_t pos = 0) noexcept {
        reserve(pos + len);
        std::memcpy(begin() + pos, buf, len);
        return len;
    }

    void reserve(size_t n) noexcept {
        if (capacity() >= n) {
            return;
        }
        size_t sz = size();
        if (block_.size() >= n && begin_ > 0) {
            if (sz > 0) {
                std::memmove(&block_[0], begin(), sz);
            }
            begin_ = 0;
            end_ = sz;
            return;
        }
        size_t new_size = (std::max)(block_.size(), MIN_BLOCK_SIZE);
        while (new_size < n)
            new_size += new_size;
        block_.resize(new_size);
    }

    TINYNET_FORCEINLINE char* prepare(size_t n) noexcept {
        reserve(size() + n);
        return end();
    }

    TINYNET_FORCEINLINE void resize(size_t n) noexcept {
        if (n > capacity()) {
            reserve(n);
        }
        end_ = begin_ + n;
    }

    TINYNET_FORCEINLINE void commit(size_t n) noexcept {
        end_ = (std::min)(end_ + n, block_.size());
    }

    TINYNET_FORCEINLINE void consume(size_t n) noexcept {
        begin_ += n;
        if (begin_ >= end_) {
            begin_ = end_ = 0;
        }
    }

    TINYNET_FORCEINLINE size_t find(char ch, size_t pos = 0) {
        if (pos < size()) {
            char* p = (char*)std::memchr(begin() + pos, ch, size());
            return p == NULL ? npos : p - begin();
        }
        return npos;

    }

    TINYNET_FORCEINLINE char* begin() noexcept { return &block_[0] + begin_; }


    TINYNET_FORCEINLINE char* end() noexcept { return &block_[0] + end_; }

    TINYNET_FORCEINLINE char *data() noexcept { return begin(); }

    void shrink_to_fit() noexcept {
        size_t sz = size();
        if (block_.size() <= MIN_BLOCK_SIZE || block_.size() < (sz << 2)) {
            return;
        }
        if (sz > 0 && begin_ > 0) {
            std::memmove(&block_[0], begin(), sz);
        }
        begin_ = 0;
        end_ = sz;
        block_.resize(block_.size() >> 1);
        block_.shrink_to_fit();
    }

    TINYNET_FORCEINLINE size_t size() const noexcept { return (size_t)(end_ - begin_); }

    TINYNET_FORCEINLINE bool empty() const noexcept { return begin_ == end_; }

    TINYNET_FORCEINLINE size_t capacity() const noexcept { return block_.size() - begin_; }

    TINYNET_FORCEINLINE void clear() noexcept { begin_ = end_ = 0; }

    TINYNET_FORCEINLINE size_t max_size() const noexcept { return block_.max_size(); }

    TINYNET_FORCEINLINE char& operator[] (size_t pos) noexcept { return block_[begin_ + pos]; }

    TINYNET_FORCEINLINE const char& operator[] (size_t pos) const noexcept { return block_[begin_ + pos]; }

    std::string tostring() {
        std::string hex_str(size() * 3, 0);
        for (size_t i = begin_; i < end_; ++i) {
            sprintf(&hex_str[i * 3], "%02x ", (unsigned char)block_[i]);
        }
        return hex_str;
    }

    void swap(IOBuffer& other) {
        std::swap(this->block_, other.block_);
        std::swap(this->begin_, other.begin_);
        std::swap(this->end_, other.end_);
    }
  private:
    std::vector<char> block_;
    size_t begin_{ 0 };
    size_t end_{ 0 };
};

}