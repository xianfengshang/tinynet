// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "base/json_types.h"

namespace redis {

const int kDefaultConnectTimeout = 20000;

struct RedisOptions {
    std::string host;
    int port{ 6379 };
    std::string path;
    std::string password;
    int timeout{ kDefaultConnectTimeout };
    bool debug{ false };
};

enum RedisReplyType {
    REDIS_REPLY_UNKNOWN,
    REDIS_REPLY_STATUS,
    REDIS_REPLY_ERROR,
    REDIS_REPLY_STRING,
    REDIS_REPLY_NIL,
    REDIS_REPLY_INTEGER,
    REDIS_REPLY_ARRAY
};

struct RedisReply {
    RedisReply() :type(REDIS_REPLY_UNKNOWN), integer(0) { };

    RedisReply(const RedisReply& o) {
        *this = o;
    }
    RedisReply(RedisReply&& o) {
        *this = std::move(o);
    }
    RedisReply& operator=(const RedisReply& o) noexcept {
        type = o.type;
        switch (o.type) {
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_ERROR:
        case REDIS_REPLY_STRING:
            str = o.str;
            break;
        case REDIS_REPLY_NIL:
            break;
        case REDIS_REPLY_INTEGER:
            integer = o.integer;
            break;
        case REDIS_REPLY_ARRAY:
            element = o.element;
            break;
        default:
            break;
        }
        return *this;
    }
    RedisReply& operator=(RedisReply&& o) noexcept {
        type = o.type;
        switch (o.type) {
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_ERROR:
        case REDIS_REPLY_STRING:
            str = std::move(o.str);
            break;
        case REDIS_REPLY_NIL:
            break;
        case REDIS_REPLY_INTEGER:
            integer = o.integer;
            break;
        case REDIS_REPLY_ARRAY:
            element = std::move(o.element);
            break;
        default:
            break;
        }
        o.type = REDIS_REPLY_UNKNOWN;
        return *this;
    }
    void clear() {
        switch (type) {
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_ERROR:
        case REDIS_REPLY_STRING:
            str.clear();
            str.shrink_to_fit();
            break;
        case REDIS_REPLY_NIL:
            break;
        case REDIS_REPLY_INTEGER:
            integer = 0;
            break;
        case REDIS_REPLY_ARRAY:
            element.clear();
            element.shrink_to_fit();
            break;
        default:
            break;
        }
        type = REDIS_REPLY_UNKNOWN;
    }
    int type;
    int64_t integer;
    std::string str;
    std::vector<RedisReply> element;
};

typedef std::function<void(const RedisReply& reply)> RedisCallback;

}

inline const tinynet::json::Value& operator << (tinynet::json::Value& json_value, const redis::RedisReply& o);

template<> inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const std::vector<redis::RedisReply>& o) {
    if (!json_value.IsArray()) json_value.SetArray();

    for (size_t i = 0; i < o.size(); ++i) {
        tinynet::json::Value value;
        value << o[i];
        json_value.PushBack(value, tinynet::json::g_allocator);
    }
    return json_value;
}

inline const tinynet::json::Value& operator << (tinynet::json::Value& json_value, const redis::RedisReply& o) {
    switch (o.type) {
    case redis::REDIS_REPLY_STATUS:
        json_value << o.str;
        break;
    case redis::REDIS_REPLY_ERROR:
        json_value << o.str;
        break;
    case redis::REDIS_REPLY_INTEGER:
        json_value << o.integer;
        break;
    case redis::REDIS_REPLY_NIL:
        json_value.SetNull();
        break;
    case redis::REDIS_REPLY_STRING:
        json_value << o.str;
        break;
    case redis::REDIS_REPLY_ARRAY:
        json_value << o.element;
        break;
    default:
        break;
    }
    return json_value;
}
