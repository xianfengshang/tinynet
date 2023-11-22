// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include "http_protocol.h"
#include "net/socket.h"
#include "zlib.h"
#include "http_parser.h"

namespace tinynet {
namespace http {

class HttpCodec {
  public:
    HttpCodec();
    HttpCodec(const HttpCodec&) = delete;
    HttpCodec(HttpCodec&&) = delete;
    HttpCodec& operator=(const HttpCodec&) = delete;
  private:
    enum HeaderStatus { NOTHING, FIELD, VALUE };
  public:
    bool Read(net::SocketPtr& sock, std::vector<std::unique_ptr<HttpMessage> > *outputs);
    void Write(net::SocketPtr& sock, const HttpMessage *msg);
    void Write(net::SocketPtr& sock, const ZeroCopyHttpMessage *msg);
  public:
    static int s_on_message_begin(http_parser * parser) {
        auto self = reinterpret_cast<HttpCodec*>(parser->data);
        return self->on_message_begin();
    }
    static int s_on_url(http_parser * parser, const char *at, size_t length) {
        auto self = reinterpret_cast<HttpCodec*>(parser->data);
        return self->on_url(at, length);
    }
    static int s_on_header_field(http_parser * parser, const char *at, size_t length) {
        auto self = reinterpret_cast<HttpCodec*>(parser->data);
        return self->on_header_field(at, length);
    }
    static int s_on_header_value(http_parser * parser, const char *at, size_t length) {
        auto self = reinterpret_cast<HttpCodec*>(parser->data);
        return self->on_header_value(at, length);
    }
    static int s_on_headers_complete(http_parser * parser) {
        auto self = reinterpret_cast<HttpCodec*>(parser->data);
        return self->on_headers_complete();
    }
    static int s_on_body(http_parser * parser, const char *at, size_t length) {
        auto self = reinterpret_cast<HttpCodec*>(parser->data);
        return self->on_body(at, length);
    }
    static int s_on_message_complete(http_parser * parser) {
        auto self = reinterpret_cast<HttpCodec*>(parser->data);
        return self->on_message_complete();
    }
  private:
    int on_message_begin();
    int on_url(const char *at, size_t length);
    int on_header_field(const char *at, size_t length);
    int on_header_value(const char *at, size_t length);
    int on_headers_complete();
    int on_body(const char *at, size_t length);
    int on_message_complete();
  public:
    using HttpMessagePtr = std::unique_ptr<HttpMessage>;
  private:
    struct http_parser parser_;
    HttpMessagePtr message_;	// Current message
    std::vector<HttpMessagePtr> messages_; //Parsed messages
    HeaderStatus header_status_;
    std::string header_field_;
    std::string header_value_;
    z_stream zs_;
    bool unzip_;
    bool zfail_;
    bool zinit_;
};
}
}
