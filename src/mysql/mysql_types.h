// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <vector>
#include "base/json_types.h"
namespace mysql {

const int kDefaultConnectionLimit = 10;

const int kDefaultConnectTimeout = 10000;

struct Config {
    std::string	host;
    int			port{ 3306 };
    std::string	user;
    std::string	password;
    std::string	database;
    std::string	unix_socket;
    unsigned long	flags{ 0 };
    int connectionLimit{ kDefaultConnectionLimit };
    std::string encoding;
    bool connectionReuse{ true };
    bool debug{ false };
    bool logConnect{ true };
    int connectTimeout{ kDefaultConnectTimeout };
    bool compress{ false };
    bool use_ssl{ false };
    bool use_io_thread{ false };
};

struct DataColumn {
    std::string name;
    int type{ 0 };
};

typedef std::vector<DataColumn> DataColumns;
typedef std::vector<std::string> DataRow;
typedef std::vector<DataRow> DataRows;

struct QueryRequest {
    std::string sql;
};

struct QueryResult {
    int64_t insertId{ 0 };
    int32_t affectedRows{ 0 };
    DataColumns fields;
    DataRows rows;
};

struct QueryResponse {
    int32_t code{ 0 };
    std::string msg;
    std::vector<QueryResult> results;
};

}
/*
template<> inline const tinynet::json::Value& operator << (const tinynet::json::Value& json_value, std::vector<std::string, config::ServiceInfo>& o) {
	o.clear();
	tinynet::json::Value::Members keys = json_value.getMemberNames();
	for (tinynet::json::Value::Members::iterator it = keys.begin();
		it != keys.end(); ++it) {
		const std::string& key = *it;
		const tinynet::json::Value& value = json_value[key];
		config::ServiceInfo v;
		value >> v;
		o.insert(std::make_pair(key, v));
	}
	return json_value;
}*/

inline const tinynet::json::Value& operator << (tinynet::json::Value& json_value, const mysql::DataColumn& o) {
    JSON_WRITE_FIELD(name);
    JSON_WRITE_FIELD(type);
    return json_value;
}

template<>
inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const std::vector<mysql::DataColumn>& o) {
    if (!json_value.IsArray()) json_value.SetArray();

    for (std::vector<mysql::DataColumn>::const_iterator it = o.begin();
            it != o.end(); ++it) {
        tinynet::json::Value value;
        value << *it;
        json_value.PushBack(value, tinynet::json::g_allocator);
    }
    return json_value;
}

inline const tinynet::json::Value& operator << (tinynet::json::Value& json_value, const mysql::QueryResult& o) {
    JSON_WRITE_FIELD(insertId);
    JSON_WRITE_FIELD(affectedRows);
    JSON_WRITE_FIELD(fields);
    JSON_WRITE_FIELD(rows);
    return json_value;
}

template<>
inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const std::vector<mysql::QueryResult>& o) {
    if (!json_value.IsArray()) json_value.SetArray();

    for (std::vector<mysql::QueryResult>::const_iterator it = o.begin();
            it != o.end(); ++it) {
        tinynet::json::Value value;
        value << *it;
        json_value.PushBack(value, tinynet::json::g_allocator);
    }
    return json_value;
}

inline const tinynet::json::Value& operator << (tinynet::json::Value& json_value, const mysql::QueryResponse& o) {
    JSON_WRITE_FIELD(code);
    JSON_WRITE_FIELD(msg);
    JSON_WRITE_FIELD(results);
    return json_value;
}

inline const tinynet::json::Value& operator << (tinynet::json::Value& json_value, const mysql::Config& o) {
    JSON_WRITE_FIELD(host);
    JSON_WRITE_FIELD(port);
    JSON_WRITE_FIELD(user);
    JSON_WRITE_FIELD(password);
    JSON_WRITE_FIELD(database);
    JSON_WRITE_FIELD(unix_socket);
    //WRITE_FIELD(flags);
    JSON_WRITE_FIELD(connectionLimit);
    JSON_WRITE_FIELD(connectionReuse);
    JSON_WRITE_FIELD(debug);
    JSON_WRITE_FIELD(logConnect);
    JSON_WRITE_FIELD(use_ssl);
    JSON_WRITE_FIELD(use_io_thread);
    return json_value;
}
