// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <map>
namespace UriUtils {
struct uri_info {
    std::string scheme;
    std::string username;
    std::string password;
    std::string host;
    int port{ 0 };
    std::string path;
    std::map<std::string, std::string> query;
    std::string fragment;
};

bool parse_uri(const std::string& uri, uri_info& out_info);
bool parse_address(const std::string& uri, std::string* host_out, int* port_out);
bool parse_path(const std::string& uri, std::string* path_out);

std::string& format_address(std::string& dst, const char* protocol, const std::string& host, const int* port);
}
