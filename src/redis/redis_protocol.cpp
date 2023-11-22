#include "redis_protocol.h"
namespace redis {
namespace protocol {

const char* FindCrlf(const char* buff, size_t len) {
    if (len < 2) return NULL;
    size_t scan_len = len - 1;
    for (size_t pos = 0; pos < scan_len; ++pos) {
        if (buff[pos] == '\r' && buff[pos + 1] == '\n') {
            return &buff[pos];
        }
    }
    return NULL;
}

bool ReadInteger(const char* buff, size_t len, int64_t* integer) {
    char* endptr = NULL;
    *integer = std::strtoll(buff, &endptr, 10);
    if (endptr != buff + len) {
        return false;
    }
    return true;
}

bool ReadInteger(tinynet::IBufferStream& stream, size_t len, int64_t* integer) {
    char decimals[21] = { 0 }; //enough to hold large int64 decimal
    if (len > sizeof(decimals) - 1) {
        return false;
    }
    stream.read(decimals, len);
    char* endptr = NULL;
    *integer = std::strtoll(decimals, &endptr, 10);
    if (endptr != decimals + len) {
        return false;
    }
    return true;
}

bool WriteCommand(tinynet::IOBuffer* io_buf, const char* cmd, size_t len) {
    const char* last = cmd + len;
    std::vector<tinynet::string_view> iovs;
    char quote_mark = 0;
    const char* begin = NULL;
    for (const char* p = cmd; p != last; ++p) {
        if (*p == ' ') {
            if (!quote_mark && begin != NULL) {
                iovs.emplace_back(begin, p - begin);
                begin = NULL;
            }
        } else {
            if (begin == NULL) {
                begin = p;
            }
            if (*p == '"' || *p == '\'') {
                if (quote_mark == 0) {
                    quote_mark = *p;
                } else if (quote_mark == *p) {
                    quote_mark = 0;
                }
            }
        }
    }
    if (!quote_mark && begin != NULL) {
        iovs.emplace_back(begin, last - begin);
    }
    return WriteCommand(io_buf, iovs);
}


static size_t GetWriteSize(const std::vector<tinynet::string_view>& iovs) {
    size_t n = 0;
    if (iovs.size() == 0)
        return n;
    n += 1; //*
    n += snprintf(NULL, 0, "%lld", (long long)iovs.size()); //nargs
    n += 2; //crlf
    for (auto& iov : iovs) {
        n += 1; //$
        n += snprintf(NULL, 0, "%lld", (long long)iov.size());
        n += 2; //crlf;
        n += iov.size(); //payload len
        n += 2; //crlf;
    }
    return n;
}

bool WriteCommand(tinynet::IOBuffer* io_buf, const std::vector<tinynet::string_view>& iovs) {
    if (iovs.size() == 0)
        return false;
    size_t len = GetWriteSize(iovs);
    char* first = io_buf->prepare(len + 1);
    char* last = first + len + 1;
    char* p = first;
    *p++ = '*';
    p += snprintf(p, last - p, "%lld", (long long)iovs.size());
    *p++ = '\r';
    *p++ = '\n';
    for (auto& iov : iovs) {
        *p++ = '$';
        p += snprintf(p, last - p, "%lld", (long long)iov.size());
        *p++ = '\r';
        *p++ = '\n';
        for (size_t i = 0; i < iov.size(); i++) {
            *p++ = iov[i];
        }
        *p++ = '\r';
        *p++ = '\n';
    }
    io_buf->commit(p - first);
    return true;
}
}
}
