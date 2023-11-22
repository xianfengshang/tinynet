// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <vector>
#include "base/json_types.h"

namespace tinynet {
namespace raft {

enum StateType {
    Unkown,
    Leader,
    Candidate,
    Follower
};

const int kNilNode = -1;

const uint64_t kNilLogIndex = 0;

const uint64_t kNilLogTerm = 0;

struct PeerConfig {
    std::string name;
    std::string url;
};

struct NodeConfig {
    int id{ kNilNode };
    bool standalong { false };
    bool debugMode{ false };
    std::string dataDir;
    int snapshotCount{ 0 };
    int heartbeatInterval{ 0 };
    int electionTimeout{ 0 };
    std::vector<std::string> peers;
};

struct LogEntry {
    uint64_t index{ kNilLogIndex };
    uint64_t term{ kNilLogTerm };
    std::string data;
};

}
}

inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const tinynet::raft::PeerConfig& o) {
    JSON_WRITE_FIELD (name);
    JSON_WRITE_FIELD(url);
    return json_value;
}

template <> inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const std::vector<tinynet::raft::PeerConfig>& o) {
    if (json_value.IsArray()) json_value.SetArray();

    for (std::vector<tinynet::raft::PeerConfig>::const_iterator it = o.begin();
            it != o.end(); ++it) {
        tinynet::json::Value value;
        value << *it;
        json_value.PushBack(value, tinynet::json::g_allocator);
    }
    return json_value;
}

inline tinynet::json::Value& operator << (tinynet::json::Value& json_value, const tinynet::raft::NodeConfig& o) {
    JSON_WRITE_FIELD(id);
    JSON_WRITE_FIELD(standalong);
    JSON_WRITE_FIELD(debugMode);
    JSON_WRITE_FIELD(dataDir);
    JSON_WRITE_FIELD(snapshotCount);
    JSON_WRITE_FIELD(heartbeatInterval);
    JSON_WRITE_FIELD(electionTimeout);
    JSON_WRITE_FIELD(peers);
    return json_value;
}
