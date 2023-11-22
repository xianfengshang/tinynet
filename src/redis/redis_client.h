// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include "net/event_loop.h"
#include "redis_types.h"
#include "base/string_view.h"

namespace redis {

class RedisChannel;
typedef std::shared_ptr<RedisChannel> RedisChannelPtr;

class RedisClient {
  public:
    RedisClient(tinynet::EventLoop *loop);
    ~RedisClient();
  public:
    RedisClient(const RedisClient&) = delete;
    RedisClient(RedisClient&&) = delete;
    RedisClient& operator=(const RedisClient&) = delete;
  public:
    void Init(const RedisOptions& options);
    int Command(const tinynet::string_view& cmd, RedisCallback cb);
    int CommandArgv(const std::vector<tinynet::string_view>& iovs, RedisCallback cb);
    int Monitor(RedisCallback cb);
    int Subscribe(const std::string& name, RedisCallback cb);
    int Psubscribe(const std::string& pattern, RedisCallback cb);
    RedisChannelPtr CreateSubscribeChannel(const std::string& name);
    RedisChannelPtr GetSubscribeChannel(const std::string& name);
    void Unsubscribe(const std::string& name);
    void Punsubscribe(const std::string& pattern);
  private:
    tinynet::EventLoop* event_loop_;
    RedisOptions options_;
    RedisChannelPtr command_channel_;
    RedisChannelPtr monitor_channel_;
    std::unordered_map<std::string, RedisChannelPtr> subscribers_;
};
}
