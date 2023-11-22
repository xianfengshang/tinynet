// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <vector>
#include "base/json_types.h"

namespace tinynet {
namespace app {
struct AppMeta {
    std::string name;
    std::map<std::string, std::string> labels;
};
}
}

inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, tinynet::app::AppMeta& o) {
    JSON_READ_FIELD(name);
    JSON_READ_FIELD(labels);
    return json_value;
}

template<> inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, std::vector<tinynet::app::AppMeta>& o) {
    o.clear();
    if (json_value.IsArray()) {
        for (tinynet::json::SizeType i = 0; i < json_value.Size(); ++i) {
            tinynet::app::AppMeta item;
            json_value[i] >> item;
            o.emplace_back(std::move(item));
        }
    }
    return json_value;
}
