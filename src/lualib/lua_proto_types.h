// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "net/http/http_client.h"
#include "net/http/http_server.h"
#include "net/http/http_protocol.h"
#include "redis/redis_types.h"
#include "lua_types.h"
#include "lua_script.h"
#include "mysql/mysql_types.h"
#include "logging/logging.h"
#include "lua_identifier.h"
#include "lua_websocket_types.h"
#include "app/app_types.h"
#include "cluster/cluster_service.h"
#include "naming/naming_resolver.h"
#include "lua_process_types.h"
#include "lua_socket_types.h"
#include "tfs/tfs_types.h"
#include "process/process_types.h"
#include "lua_common_types.h"
#include "net/socket_channel.h"
#include "net/socket_server.h"
#include "base/vector3.h"
#include "base/vector2.h"
#include "base/vector3int.h"
#include "base/bounds.h"

#define  LUA_READ_IDENTIFIER(NAME)\
	lua_pushstring(L.L, #NAME);\
	lua_rawget(L.L, -2);\
	o.NAME = luaL_toidentifier(L.L, -1);\
	lua_pop(L.L, 1)

#define LUA_WRITE_IDENTIFIER(NAME)\
	lua_pushstring(L.L, #NAME);\
	lua_pushidentifier(L.L, o.NAME);\
	lua_rawset(L.L, -3)

inline const LuaState& operator >> (const LuaState& L, tinynet::http::client::AuthConfig & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(username);
    LUA_READ_FIELD(password);
    LUA_READ_END();
}

inline LuaState& operator << (LuaState& L, const tinynet::http::client::AuthConfig & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(username);
    LUA_WRITE_FIELD(password);
    LUA_WRITE_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::http::client::ProxyConfig & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(url);
    LUA_READ_FIELD(auth);
    LUA_READ_END();
}

inline LuaState& operator << (LuaState& L, const tinynet::http::client::ProxyConfig & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(url);
    LUA_WRITE_FIELD(auth);
    LUA_WRITE_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::http::client::Request & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(url);
    LUA_READ_FIELD(method);
    LUA_READ_FIELD(baseUrl);
    LUA_READ_FIELD(headers);
    LUA_READ_FIELD(params);
    LUA_READ_FIELD(data);
    LUA_READ_FIELD(timeout);
    LUA_READ_FIELD(withCredentials);
    LUA_READ_FIELD(proxy);
    LUA_READ_FIELD(maxContentLength);
    LUA_READ_FIELD(maxRedirects);
    LUA_READ_FIELD(verbose);
    LUA_READ_END();
}
inline LuaState& operator << (LuaState& L, const tinynet::http::client::Request & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(url);
    LUA_WRITE_FIELD(method);
    LUA_WRITE_FIELD(baseUrl);
    LUA_WRITE_FIELD(headers);
    LUA_WRITE_FIELD(params);
    LUA_WRITE_FIELD(data);
    LUA_WRITE_FIELD(timeout);
    LUA_WRITE_FIELD(withCredentials);
    LUA_WRITE_FIELD(proxy);
    LUA_WRITE_FIELD(maxContentLength);
    LUA_WRITE_FIELD(maxRedirects);
    LUA_WRITE_FIELD(verbose);
    LUA_WRITE_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::http::client::Response & o) {
    LUA_READ_BEGIN();
    LUA_READ_IDENTIFIER(guid);
    LUA_READ_FIELD(code);
    LUA_READ_FIELD(msg);
    LUA_READ_FIELD(data);
    LUA_READ_FIELD(status);
    LUA_READ_FIELD(headers);
    LUA_READ_FIELD(cookies);
    LUA_READ_END();
}

inline LuaState& operator << (LuaState& L, const tinynet::http::client::Response & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_IDENTIFIER(guid);
    LUA_WRITE_FIELD(code);
    LUA_WRITE_FIELD(msg);
    LUA_WRITE_FIELD(data);
    LUA_WRITE_FIELD(status);
    LUA_WRITE_FIELD(headers);
    LUA_WRITE_FIELD(cookies);
    LUA_WRITE_END();
}

inline LuaState& operator << (LuaState& L, const redis::RedisReply & o) {
    switch (o.type) {
    case redis::REDIS_REPLY_STATUS:
        L << o.str;
        break;
    case redis::REDIS_REPLY_ERROR:
        L << o.str;
        break;
    case redis::REDIS_REPLY_INTEGER:
        L << o.integer;
        break;
    case redis::REDIS_REPLY_NIL:
        lua_pushnil(L.L);
        break;
    case redis::REDIS_REPLY_STRING:
        L << o.str;
        break;
    case redis::REDIS_REPLY_ARRAY:
        L << o.element;
        break;
    default:
        break;
    }
    return L;
}

inline LuaState& operator << (LuaState& L, const mysql::DataColumn & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(name);
    LUA_WRITE_FIELD(type);
    LUA_WRITE_END();
    return L;
}

inline LuaState& operator << (LuaState& L, const mysql::QueryResult & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(insertId);
    LUA_WRITE_FIELD(affectedRows);
    LUA_WRITE_FIELD(fields);
    LUA_WRITE_FIELD(rows);
    LUA_WRITE_END();
    return L;
}

inline LuaState& operator << (LuaState& L, const tinynet::app::AppMeta &o ) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(name);
    LUA_WRITE_FIELD(labels);
    LUA_WRITE_END();
}

inline LuaState& operator << (LuaState& L, const tinynet::lua::LuaEnv & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(meta);
    LUA_WRITE_FIELD(app);
    LUA_WRITE_END();
}


inline const LuaState& operator >> (const LuaState& L, mysql::Config & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(host);
    LUA_READ_FIELD_EX(port, 3306);
    LUA_READ_FIELD(user);
    LUA_READ_FIELD(password);
    LUA_READ_FIELD(database);
    LUA_READ_FIELD(unix_socket);
    LUA_READ_FIELD_EX(flags, 0);
    LUA_READ_FIELD_EX(connectionLimit, mysql::kDefaultConnectionLimit);
    LUA_READ_FIELD_EX(encoding, "utf8");
    LUA_READ_FIELD_EX(debug, false);
    LUA_READ_FIELD_EX(connectionReuse, true);
    LUA_READ_FIELD_EX(logConnect, true);
    LUA_READ_FIELD_EX(connectTimeout, mysql::kDefaultConnectTimeout);
    LUA_READ_FIELD_EX(compress, false);
    LUA_READ_FIELD_EX(use_ssl, false);
    LUA_READ_FIELD_EX(use_io_thread, false);
    LUA_READ_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::http::HttpMessage & o) {
    LUA_READ_BEGIN();
    LUA_READ_IDENTIFIER(guid);
    LUA_READ_FIELD_COND(url, o.type == tinynet::http::HTTP_REQUEST);
    LUA_READ_FIELD_COND(method, o.type == tinynet::http::HTTP_REQUEST);
    LUA_READ_FIELD_COND(statusCode, o.type == tinynet::http::HTTP_RESPONSE);
    LUA_READ_FIELD_COND(statusMessage, o.type == tinynet::http::HTTP_RESPONSE);
    LUA_READ_FIELD(headers);
    LUA_READ_FIELD_COND(path, o.type == tinynet::http::HTTP_REQUEST);
    LUA_READ_FIELD_COND(query, o.type == tinynet::http::HTTP_REQUEST);
    LUA_READ_FIELD(body);
    LUA_READ_FIELD(remoteAddr);
    LUA_READ_END();
}

inline LuaState& operator << (LuaState& L, const tinynet::http::HttpMessage & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_IDENTIFIER(guid);
    LUA_WRITE_FIELD_COND(url, o.type == tinynet::http::HTTP_REQUEST);
    LUA_WRITE_FIELD_COND(method, o.type == tinynet::http::HTTP_REQUEST);
    LUA_WRITE_FIELD_COND(statusCode, o.type == tinynet::http::HTTP_RESPONSE);
    LUA_WRITE_FIELD_COND(statusMessage, o.type == tinynet::http::HTTP_RESPONSE);
    LUA_WRITE_FIELD(headers);
    LUA_WRITE_FIELD_COND(path, o.type == tinynet::http::HTTP_REQUEST);
    LUA_WRITE_FIELD_COND(query, o.type == tinynet::http::HTTP_REQUEST);
    LUA_WRITE_FIELD(body);
    LUA_WRITE_FIELD(remoteAddr);
    LUA_WRITE_END();
}


inline const LuaState& operator >> (const LuaState& L, tinynet::http::ZeroCopyHttpMessage & o) {
    LUA_READ_BEGIN();
    LUA_READ_IDENTIFIER(guid);
    LUA_READ_FIELD_COND(url, o.type == tinynet::http::HTTP_REQUEST);
    LUA_READ_FIELD_COND(method, o.type == tinynet::http::HTTP_REQUEST);
    LUA_READ_FIELD_COND(statusCode, o.type == tinynet::http::HTTP_RESPONSE);
    LUA_READ_FIELD_COND(statusMessage, o.type == tinynet::http::HTTP_RESPONSE);
    LUA_READ_FIELD(headers);
    LUA_READ_FIELD_COND(path, o.type == tinynet::http::HTTP_REQUEST);
    LUA_READ_FIELD_COND(query, o.type == tinynet::http::HTTP_REQUEST);
    LUA_READ_FIELD(body);
    LUA_READ_FIELD(remoteAddr);
    LUA_READ_END();
}

inline LuaState& operator << (LuaState& L, const tinynet::http::ZeroCopyHttpMessage & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_IDENTIFIER(guid);
    LUA_WRITE_FIELD_COND(url, o.type == tinynet::http::HTTP_REQUEST);
    LUA_WRITE_FIELD_COND(method, o.type == tinynet::http::HTTP_REQUEST);
    LUA_WRITE_FIELD_COND(statusCode, o.type == tinynet::http::HTTP_RESPONSE);
    LUA_WRITE_FIELD_COND(statusMessage, o.type == tinynet::http::HTTP_RESPONSE);
    LUA_WRITE_FIELD(headers);
    LUA_WRITE_FIELD_COND(path, o.type == tinynet::http::HTTP_REQUEST);
    LUA_WRITE_FIELD_COND(query, o.type == tinynet::http::HTTP_REQUEST);
    LUA_WRITE_FIELD(body);
    LUA_WRITE_FIELD(remoteAddr);
    LUA_WRITE_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::lua::WebSocketEvent & o) {
    LUA_READ_BEGIN();
    LUA_READ_IDENTIFIER(guid);
    LUA_READ_FIELD(addr);
    LUA_READ_FIELD(type);
    LUA_READ_FIELD(data);
    LUA_READ_END();
}

inline LuaState& operator << (LuaState& L, const tinynet::lua::WebSocketEvent & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_IDENTIFIER(guid);
    LUA_WRITE_FIELD(addr);
    LUA_WRITE_FIELD(type);
    LUA_WRITE_FIELD_COND_EX(data, o.data_ref, !!o.data_ref);
    LUA_WRITE_FIELD_COND_EX(data, o.data, !o.data_ref && o.data.length() > 0);
    LUA_WRITE_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::cluster::NodeInfo & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(id);
    LUA_READ_FIELD(url);
    LUA_READ_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::cluster::NamingServiceOptions & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD_EX(debugMode, false);
    LUA_READ_FIELD(nameSpace);
    LUA_READ_FIELD_EX(registrationInterval, 0);
    LUA_READ_FIELD_EX(expiryTime, 0);
    LUA_READ_FIELD(dataDir);
    LUA_READ_FIELD_EX(snapshotCount, 0);
    LUA_READ_FIELD_EX(heartbeatInterval, 0);
    LUA_READ_FIELD_EX(electionTimeout, 0);
    LUA_READ_FIELD(servers);
    LUA_READ_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::cluster::ClusterOptions & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(clusterName);
    LUA_READ_FIELD_EX(clusterId, 0);
    LUA_READ_FIELD(servicePortRange);
    LUA_READ_FIELD_EX(workerProcesses, 0);
    LUA_READ_FIELD(namingService);
    LUA_READ_FIELD_EX(bytesAsString, false);
    LUA_READ_END();
}

inline const LuaState& operator >> (const LuaState& L, redis::RedisOptions& o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD_EX(host, "127.0.0.1");
    LUA_READ_FIELD_EX(port, 6379);
    LUA_READ_FIELD_EX(path, "");
    LUA_READ_FIELD_EX(password, "");
    LUA_READ_FIELD_EX(timeout, 0);
    LUA_READ_END();
}
inline LuaState& operator << (LuaState& L, const redis::RedisOptions & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(host);
    LUA_WRITE_FIELD(port);
    LUA_WRITE_FIELD(path);
    LUA_WRITE_FIELD(password);
    LUA_WRITE_FIELD(timeout);
    LUA_WRITE_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::lua::ProcessEvent & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(type);
    LUA_READ_FIELD_COND(err, o.type == "onerror");
    LUA_READ_FIELD_COND(exit_status, o.type == "onexit");
    LUA_READ_FIELD_COND(term_signal, o.type == "onexit");
    LUA_READ_END();
}

inline LuaState& operator << (LuaState& L, const tinynet::lua::ProcessEvent & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(type);
    LUA_WRITE_FIELD_COND(err, o.type == "onerror");
    LUA_WRITE_FIELD_COND(exit_status, o.type == "onexit");
    LUA_WRITE_FIELD_COND(term_signal, o.type == "onexit");
    LUA_WRITE_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::process::ProcessOptions& o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(file);
    LUA_READ_FIELD(args);
    LUA_READ_FIELD(env);
    LUA_READ_FIELD_EX(cwd, "");
    LUA_READ_FIELD_EX(flags, 0);
    LUA_READ_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::lua::TcpSocketEvent & o) {
    LUA_READ_BEGIN();
    LUA_READ_IDENTIFIER(guid);
    LUA_READ_FIELD(type);
    LUA_READ_FIELD(data);
    LUA_READ_END();
}

inline LuaState& operator << (LuaState& L, const tinynet::lua::TcpSocketEvent & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_IDENTIFIER(guid);
    LUA_WRITE_FIELD(type);
    LUA_WRITE_FIELD_COND(data, o.data.length() > 0);
    LUA_WRITE_END();
}

inline LuaState& operator << (LuaState& L, const tinynet::tfs::DirectoryEntry & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(name);
    LUA_WRITE_FIELD(type);
    LUA_WRITE_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::lua::ClientOptions & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD_EX(url, "");
    LUA_READ_FIELD_EX(keepalive_timeout, 0);
    LUA_READ_FIELD_EX(ping_interval, 0);
    LUA_READ_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::lua::ServerOptions & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD_EX(listen_url, "");
    LUA_READ_FIELD_COND(listen_ip, o.listen_url.empty());
    LUA_READ_FIELD_COND(listen_port, o.listen_url.empty());
    LUA_READ_FIELD_EX(keepalive_timeout, 0);
    LUA_READ_FIELD_EX(reuseport, false);
    LUA_READ_FIELD_EX(ipv6only, false);
    LUA_READ_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::lua::JsonCodecOptions & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD_EX(encode_max_depth, 0);
    LUA_READ_FIELD_EX(decode_max_depth, 0);
    LUA_READ_FIELD_EX(order_map_entries, false);
    LUA_READ_FIELD_EX(empty_table_as_array, false);
    LUA_READ_FIELD_EX(pretty_format, false);
    LUA_READ_FIELD_EX(encode_as_bytes, false);
    LUA_READ_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::lua::ProtobufCodecOptions & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD_EX(encode_as_bytes, false);
    LUA_READ_FIELD_EX(bytes_as_string, false);
    LUA_READ_END();
}

inline LuaState& operator << (LuaState& L, const tinynet::lua::MemoryStats & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(allocated);
    LUA_WRITE_FIELD(active);
    LUA_WRITE_FIELD(resident);
    LUA_WRITE_FIELD(mapped);
    LUA_WRITE_FIELD(retained);
    LUA_WRITE_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::net::ServerOptions & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD_EX(name, "");
    LUA_READ_FIELD_EX(cert_file, "");
    LUA_READ_FIELD_EX(key_file, "");
    LUA_READ_FIELD_EX(listen_path, "");
    LUA_READ_FIELD_EX(listen_url, "");
    LUA_READ_FIELD_COND(listen_ip, o.listen_url.empty());
    LUA_READ_FIELD_COND(listen_port, o.listen_url.empty());
    LUA_READ_FIELD_EX(keepalive_ms, 0);
    LUA_READ_FIELD_EX(reuseport, false);
    LUA_READ_FIELD_EX(ipv6only, false);
    LUA_READ_FIELD_EX(debug, false);
    LUA_READ_FIELD_EX(max_packet_size, 0);
    LUA_READ_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::net::ChannelOptions & o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD_EX(name, "");
    LUA_READ_FIELD_EX(url, "");
    LUA_READ_FIELD_EX(path, "");
    LUA_READ_FIELD_COND(host, o.url.empty() && o.path.empty());
    LUA_READ_FIELD_COND(port, o.url.empty() && o.path.empty());
    LUA_READ_FIELD_EX(keepalive_ms, 0);
    LUA_READ_FIELD_EX(ping_interval, 0);
    LUA_READ_FIELD_EX(use_ssl, false);
    LUA_READ_FIELD_EX(debug, false);
    LUA_READ_END();
}


inline const LuaState& operator >> (const LuaState& L, tinynet::Vector3& o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD_EX(x, 0);
    LUA_READ_FIELD_EX(y, 0);
    LUA_READ_FIELD_EX(z, 0);
    LUA_READ_END();
}
inline LuaState& operator << (LuaState& L, tinynet::Vector3 & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(x);
    LUA_WRITE_FIELD(y);
    LUA_WRITE_FIELD(z);
    LUA_WRITE_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::Vector2& o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD_EX(x, 0);
    LUA_READ_FIELD_EX(y, 0);
    LUA_READ_END();
}
inline LuaState& operator << (LuaState& L, tinynet::Vector2 & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(x);
    LUA_WRITE_FIELD(y);
    LUA_WRITE_END();
}

inline const LuaState& operator >> (const LuaState& L, tinynet::Vector3Int& o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD_EX(x, 0);
    LUA_READ_FIELD_EX(y, 0);
    LUA_READ_FIELD_EX(z, 0);
    LUA_READ_END();
}
inline LuaState& operator << (LuaState& L, const tinynet::Vector3Int & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(x);
    LUA_WRITE_FIELD(y);
    LUA_WRITE_FIELD(z);
    LUA_WRITE_END();
}