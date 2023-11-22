// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <utility>
#include "base.h"
#include "allocator.h"
#include "io_buffer.h"

#define RAPIDJSON_HAS_STDSTRING 1

#if  defined USE_MIMALLOC
#define RAPIDJSON_MALLOC(size) mi_malloc(size)

#define RAPIDJSON_REALLOC(ptr, new_size) mi_realloc(ptr, new_size)
#define RAPIDJSON_FREE(ptr) mi_free(ptr)
#elif defined USE_JEMALLOC
#define RAPIDJSON_MALLOC(size) je_malloc(size)

#define RAPIDJSON_REALLOC(ptr, new_size) je_realloc(ptr, new_size)
#define RAPIDJSON_FREE(ptr) je_free(ptr)
#endif

#include "rapidjson/rapidjson.h"
#include "rapidjson/reader.h"
#include "rapidjson/encodings.h"
#include "rapidjson/error/en.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/memorystream.h"
#include "rapidjson/memorybuffer.h"

namespace tinynet {
namespace json {

typedef rapidjson::Document Document;

typedef rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> Allocator;
typedef rapidjson::SizeType SizeType;
extern Allocator g_allocator;
typedef rapidjson::Value Value;

std::string tojson(Value& value, bool pretty = false);
}
}
#define JSON_READ_FIELD(NAME) \
	if (json_value.IsObject() && json_value.HasMember(#NAME))\
		json_value[#NAME] >> o.NAME;

#define JSON_READ_FIELD_EX(NAME, DefaultValue) \
	if (json_value.IsObject() && json_value.HasMember(#NAME))\
		json_value[#NAME] >> o.NAME;\
	else\
		o.NAME = DefaultValue;

#define JSON_WRITE_FIELD(NAME) \
	if(!json_value.IsObject()) json_value.SetObject();\
	tinynet::json::Value json_object_field_##NAME;\
	json_object_field_##NAME << o.NAME;\
	json_value.AddMember(#NAME, json_object_field_##NAME, tinynet::json::g_allocator)

#define JSON_WRITE_ARRAY_FIELD(NAME) \
	json_value::Value json_array_element_##NAME;\
	json_array_element_##NAME << o.NAME; \
	if(!json_value.IsArray()) json_value.SetArray();\
	json_value.PushBack(json_array_element_##NAME, tinynet::json::g_allocator);

inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const bool& o) {
    json_value = o;
    return json_value;
}

inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, bool& o) {
    if (json_value.IsBool())
        o = json_value.GetBool();
    return json_value;
}

inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const int& o) {
    json_value = o;
    return json_value;
}

inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, int& o) {
    if (json_value.IsInt())
        o = json_value.GetInt();
    return json_value;
}

inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const int64_t& o) {
    json_value = o;
    return json_value;
}

inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, int64_t& o) {
    if (json_value.IsInt64())
        o = json_value.GetInt64();
    return json_value;
}

inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const std::string& o) {
    json_value.SetString(o, tinynet::json::g_allocator);
    return json_value;
}

inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, std::string& o) {
    if (json_value.IsString())
        o.assign(json_value.GetString(), json_value.GetStringLength());
    return json_value;
}

template<typename T>
inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const std::vector<T>& o) {
    if (!json_value.IsArray()) json_value.SetArray();

    for (typename std::vector<T>::const_iterator it = o.begin();
            it != o.end(); ++it) {
        tinynet::json::Value value;
        value << *it;
        json_value.PushBack(value, tinynet::json::g_allocator);
    }
    return json_value;
}

template<typename T>
inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, std::vector<T>& o) {
    o.clear();
    if (json_value.IsArray()) {
        for (tinynet::json::SizeType i = 0; i < json_value.Size(); ++i) {
            T item;
            json_value[i] >> item;
            o.emplace_back(std::move(item));
        }
    }
    return json_value;
}

template<typename T>
inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, std::set<T>& o) {
    o.clear();
    if (json_value.IsArray()) {
        for (tinynet::json::SizeType i = 0; i < json_value.Size(); ++i) {
            T item;
            json_value[i] >> item;
            o.emplace(std::move(item));
        }
    }
    return json_value;
}

template<typename K, typename V>
inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const std::map<K, V>& o) {
    if (!json_value.IsObject()) json_value.SetObject();

    for (typename std::map<K, V>::const_iterator it = o.begin();
            it != o.end(); ++it) {
        const K& key = it->first;
        V& value = it->second;

        json_value.AddMember(key, value, tinynet::json::g_allocator);
    }
    return json_value;
}

template<typename K, typename V>
inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, std::map<K, V>& o) {
    o.clear();

    if (json_value.IsObject()) {
        for (tinynet::json::Value::ConstMemberIterator it = json_value.MemberBegin(); it != json_value.MemberEnd(); it++) {
            K k;
            it->name >> k;
            V v;
            it->value >> v;
            o.insert(std::make_pair(k, v));
        }
    }
    return json_value;
}

template<typename K, typename V>
inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const std::pair<K, V>& o) {
    if (!json_value.IsObject()) json_value.SetObject();

    json_value.AddMember(o->first, o.second, tinynet::json::g_allocator);
    return json_value;
}

template<typename K, typename V>
inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, std::pair<K, V>& o) {
    if (json_value.IsObject()) {
        for (tinynet::json::Value::ConstMemberIterator it = json_value.MemberBegin(); it != json_value.MemberEnd(); it++) {
            const K& key = *it;
            V v;
            it->value >> v;
            o = std::make_pair(key, v);
            break;
        }
    }
    return json_value;
}
