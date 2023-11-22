// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <map>
#include <string>
#include <cstdint>
#include <base/string_view.h>
#include <base/io_buffer.h>
namespace tinynet {
namespace http {

enum HttpMessageType {
    HTTP_REQUEST,
    HTTP_RESPONSE,
    HTTP_UNKNOWN
};

struct HttpMessage {
    int type{ HTTP_UNKNOWN };
    int64_t guid;
    std::string url;
    std::string method;
    int statusCode{ 0 };
    std::string statusMessage;
    std::map<std::string, std::string> headers;
    std::string path;
    std::string query;
    std::string body;
    std::string remoteAddr;
};

struct ZeroCopyHttpMessage {
    int type{ HTTP_UNKNOWN };
    int64_t guid{ 0 };
    tinynet::string_view url;
    tinynet::string_view method;
    int statusCode{ 0 };
    std::string statusMessage;
    std::map<tinynet::string_view, tinynet::string_view> headers;
    tinynet::string_view path;
    tinynet::string_view query;
    tinynet::string_view body;
    tinynet::string_view remoteAddr;
};
}
}