// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <algorithm>
#include <set>
#include <map>
#include "io_buffer.h"
namespace tinynet {

class IOStreamBase {
  public:
    explicit IOStreamBase(IOBuffer * io_buf):
        io_buf_(io_buf) {
    }
    virtual ~IOStreamBase() {
    }
  public:
    IOBuffer * rdbuf() {
        return io_buf_;
    }
  protected:
    IOBuffer * io_buf_;
};

class IBufferStream :
    public IOStreamBase {
  public:
    explicit IBufferStream(IOBuffer * io_buf)
        : IOStreamBase(io_buf),
          pos_g_(0) {
    }

    size_t read(void* buf, size_t n) {
        return get(buf, n);
    }
    size_t get(void* buf, size_t n) {
        size_t size = io_buf_->copy(buf, n, pos_g_);
        pos_g_ += size;
        return size;
    }
    size_t peek(void *buf, size_t n) {
        return io_buf_->copy(buf, n, pos_g_);
    }
    IBufferStream& seekg(size_t n) {
        pos_g_ = (std::min)(n, io_buf_->size());
        return *this;
    }
    size_t tellg() {
        return pos_g_;
    }
    void cut() {
        io_buf_->consume(pos_g_);
        pos_g_ = 0;
    }
    bool eof() {
        return pos_g_ >= io_buf_->size();
    }
    size_t find_crlf() {
        return io_buf_->find('\r', pos_g_);
    }
    char read_byte() {
        return (*io_buf_)[pos_g_++];
    }
  private:
    size_t pos_g_;
};

class OBufferStream:
    public IOStreamBase {
  public:
    explicit OBufferStream(IOBuffer * io_buf)
        : IOStreamBase(io_buf),
          pos_p_(io_buf->size()) {
    }
  public:
    size_t write(const void* buf, size_t n) {
        return put(buf, n);
    }
    size_t put(const void* buf, size_t n) {
        size_t size = io_buf_->put(buf, n, pos_p_);

        pos_p_ += size;
        return size;
    }
    OBufferStream& seekp(size_t n) {
        pos_p_ = n;
        return *this;
    }
    size_t tellp() {
        return pos_p_;
    }
    void commit() {
        io_buf_->commit(pos_p_);
        pos_p_ = 0;
    }
  private:
    size_t pos_p_;
};

class IOBufferStream :
    public IBufferStream,
    public OBufferStream {
  public:
    IOBufferStream(IOBuffer * io_buf) :
        IBufferStream(io_buf),
        OBufferStream(io_buf) {

    }
};
}

template <class T>
inline tinynet::IBufferStream& operator >> (tinynet::IBufferStream& stream, T& o) {
    stream.get(&o, sizeof(T));
    return stream;
}

template<class T>
inline tinynet::OBufferStream & operator << (tinynet::OBufferStream& stream, const T& o) {
    stream.put(&o, sizeof(T));
    return stream;
}

inline tinynet::OBufferStream& operator << (tinynet::OBufferStream& stream, const std::string& o) {
    uint32_t len = static_cast<uint32_t>(o.size());
    stream.put(&len, sizeof(uint32_t));
    stream.put(o.data(), len);
    return stream;
}

inline const tinynet::IBufferStream& operator >> (tinynet::IBufferStream& stream, std::string& o) {
    uint32_t len = 0;
    stream.get(&len, sizeof(uint32_t));
    o.resize(len);
    stream.get(&o[0], len);
    return stream;
}

template<typename T>
inline tinynet::OBufferStream& operator << (tinynet::OBufferStream& stream, const std::vector<T>& o) {
    uint32_t len = o.size();
    stream.put(&len, sizeof(len));
    for (auto & e : o) {
        stream << e;
    }
    return stream;
}

template<typename T>
inline tinynet::IBufferStream& operator >> (tinynet::IBufferStream& stream, std::vector<T>& o) {
    uint32_t len = 0;
    stream.get(&len, sizeof(uint32_t));
    for (size_t i = 0; i < len; ++i) {
        T e;
        stream >> e;
        o.push_back(e);
    }
    return stream;
}

template<typename T>
inline tinynet::OBufferStream& operator << (tinynet::OBufferStream& stream, const std::set<T>& o) {
    uint32_t len = o.size();
    stream.put(&len, sizeof(len));
    for (auto & e : o) {
        stream << e;
    }
    return stream;
}


template<typename T>
inline tinynet::IBufferStream& operator >> (tinynet::IBufferStream& stream, std::set<T>& o) {
    uint32_t len = 0;
    stream.get(&len, sizeof(uint32_t));
    for (size_t i = 0; i < len; ++i) {
        T e;
        stream >> e;
        o.push_back(e);
    }
    return stream;
}

template<typename K, typename V>
inline tinynet::OBufferStream& operator << (tinynet::OBufferStream& stream, const std::map<K, V>& o) {
    uint32_t len = o.size();
    stream.put(&len, sizeof(len));
    for (typename std::map<K, V>::const_iterator it = o.begin();
            it != o.end(); ++it) {
        const K& key = it->first;
        const V& value = it->second;
        stream << key;
        stream << value;
    }
    return stream;
}

template<typename K, typename V>
inline tinynet::IBufferStream& operator >> (tinynet::IBufferStream& stream, std::map<K, V>& o) {
    uint32_t len = 0;
    stream.get(&len, sizeof(uint32_t));
    for (size_t i = 0; i < len; i++) {
        K key;
        V value;
        stream >> key;
        stream >> value;
        o.insert(std::make_pair(key, value));
    }
    return stream;
}

template<typename K, typename V>
inline tinynet::OBufferStream& operator << (tinynet::OBufferStream& stream, const std::pair<K, V>& o) {
    const K& key = o->first;
    const V& value = o->second;
    stream << key;
    stream << value;
    return stream;
}

template<typename K, typename V>
inline tinynet::IBufferStream& operator >> (tinynet::IBufferStream& stream, std::pair<K, V>& o) {
    K key;
    V value;
    stream >> key;
    value >> value;
    o = std::make_pair(key, value);
    return stream;
}
