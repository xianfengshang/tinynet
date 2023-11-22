// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "util/uri_utils.h"
#include "util/string_utils.h"
namespace UriUtils {
static void parse_query(const std::string& query, uri_info& out_info) {
    std::vector<std::string> strs;;
    StringUtils::Split(query, "&", strs);
    for (auto& str : strs) {
        std::vector<std::string> arr;
        StringUtils::Split(str, "=", arr);
        if (arr.size() == 2) {
            out_info.query.emplace(arr[0], arr[1]);
        }
    }
}

static void parse_user_info(const std::string& user_info, uri_info& out_info) {
    std::vector<std::string> arr;
    StringUtils::Split(user_info, ":", arr);
    if (arr.size() > 0) {
        out_info.username = arr[0];
    }
    if (arr.size() > 1) {
        out_info.password = arr[1];
    }
}

struct UriPortReg {
    const char* name;
    unsigned int hash;
    int port;
};

#define URI_PORT_REG(PROTO, PORT) { #PROTO, StringUtils::Hash(#PROTO), PORT}

static UriPortReg well_known_ports[] = {
    URI_PORT_REG(http, 80),
    URI_PORT_REG(https, 443),
    URI_PORT_REG(ftp, 20),
    URI_PORT_REG(ssh, 22),
    URI_PORT_REG(ws, 80),
    URI_PORT_REG(wss, 443),
    URI_PORT_REG(mysql, 3306),
    URI_PORT_REG(redis, 6379),
    URI_PORT_REG(HTTP, 80),
    URI_PORT_REG(HTTPS, 443),
    URI_PORT_REG(FTP, 20),
    URI_PORT_REG(SSH, 22),
    URI_PORT_REG(WS, 80),
    URI_PORT_REG(WSS, 443),
    URI_PORT_REG(MYSQL, 3306),
    URI_PORT_REG(REDIS, 6379),
    {0, 0, 0}
};

static void parse_addr(const std::string& addr, uri_info& out_info) {
    size_t ipv6_start, ipv6_end;
    ipv6_start = addr.find('[');
    if (ipv6_start == std::string::npos) {
        ipv6_end = std::string::npos;
    } else {
        ipv6_end = addr.find(']');
    }
    size_t colon_pos = addr.find_last_of(':');
    //ipv6 address
    if (ipv6_start != std::string::npos &&ipv6_end != std::string::npos && ipv6_start < ipv6_end) {
        out_info.host = addr.substr(ipv6_start + 1, (ipv6_end - 1) - ipv6_start);
        if (colon_pos != std::string::npos && colon_pos == (ipv6_end + 1)) {
            out_info.port = std::atoi(addr.substr(colon_pos + 1).c_str());
        }
        return;
    }
    //normal address
    if (colon_pos == std::string::npos) {
        out_info.host = addr;
        out_info.port = 0;
    } else {
        out_info.host = addr.substr(0, colon_pos);
        out_info.port = std::atoi(addr.substr(colon_pos + 1).c_str());
    }
    if (out_info.port) return;
    //Try to match well-known ports
    unsigned int hash = StringUtils::Hash(out_info.scheme.c_str());
    for (UriPortReg* p = well_known_ports; p->name; p++) {
        if (p->hash == hash) {
            out_info.port = p->port;
            break;
        }
    }
}

static void parse_authority(const std::string& authority, uri_info& out_info) {
    size_t pos = authority.find_first_of('@');
    if (pos != std::string::npos) {
        parse_user_info(authority.substr(pos, pos), out_info);
        parse_addr(authority.substr(pos + 1), out_info);
    } else {
        parse_addr(authority, out_info);
    }
}

bool parse_uri(const std::string& uri, uri_info& out_info) {
    size_t scheme_begin = 0;
    size_t scheme_end = uri.find("://");
    if (scheme_end == std::string::npos) {
        return false;
    }
    out_info.scheme = uri.substr(scheme_begin, scheme_end);
    size_t fragment_begin = uri.find_first_of('#', scheme_end + 3);
    size_t fragment_end = uri.length();
    if (fragment_begin != std::string::npos) {
        out_info.fragment = uri.substr(fragment_begin + 1, fragment_end - fragment_begin - 1);
    } else {
        fragment_begin = fragment_end;
    }
    size_t query_begin = uri.find_first_of('?', scheme_end + 3);
    size_t query_end = fragment_begin;
    if (query_begin != std::string::npos) {
        std::string query = uri.substr(query_begin + 1, query_end - query_begin - 1);
        parse_query(query, out_info);
    } else {
        query_begin = query_end;
    }
    size_t path_begin = uri.find_first_of("/", scheme_end + 3);
    size_t path_end = query_begin;
    if (path_begin != std::string::npos) {
        out_info.path = uri.substr(path_begin, path_end - path_begin);
    } else {
        path_begin = path_end;
    }
    size_t authority_begin = scheme_end + 3;
    size_t authority_end = path_begin;
    if (authority_begin >= authority_end) {
        return true;
    }
    parse_authority(uri.substr(authority_begin, authority_end - authority_begin), out_info);
    return true;
}


bool  parse_address(const std::string &uri, std::string *host_out, int* port_out) {
    UriUtils::uri_info info;
    if (UriUtils::parse_uri(uri, info)) {
        if (host_out) *host_out = info.host;
        if (port_out) *port_out = info.port;
        return true;
    }
    return false;
}

bool parse_path(const std::string& uri, std::string* path_out) {
    UriUtils::uri_info info;
    if (UriUtils::parse_uri(uri, info)) {
        *path_out = info.path;
        return true;
    }
    return false;
}

std::string& format_address(std::string& dst, const char* protocol, const std::string& host, const int* port) {
    std::string host_part, port_part;
    if (host.find(':') == std::string::npos) {
        host_part = host;
    } else {
        //ipv6 address
        host_part.append(1, '[');
        host_part.append(host);
        host_part.append(1, ']');
    }
    if (port) {
        port_part.append(1, ':');
        port_part.append(std::to_string(*port));
    }
    return StringUtils::Format(dst, "%s://%s%s", protocol, host_part.c_str(), port_part.c_str());
}
}
