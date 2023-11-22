// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <vector>
#include <tuple>
#include "base/json_types.h"
namespace tinynet {
namespace cluster {

struct NodeInfo {
    std::string id;
    std::string url;

    bool operator==(const NodeInfo&) const;
};

struct NamingServiceOptions {
    bool debugMode{ false };
    std::string nameSpace;
    int registrationInterval{ 0 };
    int expiryTime{ 0 };
    std::string dataDir;
    int snapshotCount{ 0 };
    int heartbeatInterval{ 0 };
    int electionTimeout{ 0 };
    std::vector<NodeInfo> servers;
};

struct ClusterOptions {
    std::string clusterName;
    int clusterId{ 0 };
    std::tuple<int, int> servicePortRange;
    int workerProcesses;
    NamingServiceOptions namingService;
    bool bytesAsString{ false };
};

}
}

namespace tinynet {
namespace cluster {
inline bool NodeInfo::operator==(const NodeInfo& o) const {
    return id == o.id;
}
}
}
inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, tinynet::cluster::NodeInfo& o) {
    JSON_READ_FIELD(id);
    JSON_READ_FIELD(url);
    return json_value;
}

inline const tinynet::json::Value& operator << (tinynet::json::Value& json_value, const tinynet::cluster::NodeInfo& o) {
    JSON_WRITE_FIELD(id);
    JSON_WRITE_FIELD(url);
    return json_value;
}

template<> inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, std::vector<tinynet::cluster::NodeInfo>& o) {
    o.clear();
    if (json_value.IsArray()) {
        for (tinynet::json::SizeType i = 0; i < json_value.Size(); ++i) {
            tinynet::cluster::NodeInfo item;
            json_value[i] >> item;
            o.emplace_back(std::move(item));
        }
    }
    return json_value;
}


template<> inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const std::vector<tinynet::cluster::NodeInfo>& o) {
    if (!json_value.IsArray()) json_value.SetArray();
    for (auto it = o.begin(); it != o.end(); ++it) {
        tinynet::json::Value value;
        value << *it;
        json_value.PushBack(value, tinynet::json::g_allocator);
    }
    return json_value;
}

inline const tinynet::json::Value& operator >> (const tinynet::json::Value& json_value, tinynet::cluster::NamingServiceOptions& o) {
    JSON_READ_FIELD_EX(debugMode, false);
    JSON_READ_FIELD(nameSpace);
    JSON_READ_FIELD_EX(registrationInterval, 0);
    JSON_READ_FIELD_EX(expiryTime, 0);
    JSON_READ_FIELD(dataDir);
    JSON_READ_FIELD_EX(snapshotCount, 0);
    JSON_READ_FIELD_EX(heartbeatInterval, 0);
    JSON_READ_FIELD_EX(electionTimeout, 0);
    JSON_READ_FIELD(servers);
    return json_value;
}
