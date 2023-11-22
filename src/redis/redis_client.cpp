// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "redis_client.h"
#include "redis_channel.h"
#include "base/error_code.h"
namespace redis {

RedisClient::RedisClient(tinynet::EventLoop* loop):
    event_loop_(loop) {
}

RedisClient::~RedisClient() {
}

void RedisClient::Init(const RedisOptions& options) {
    options_ = options;
}

int RedisClient::Command(const tinynet::string_view& cmd, RedisCallback cb) {
    if (!command_channel_) {
        command_channel_ = std::make_shared<RedisChannel>(event_loop_);
        command_channel_->Init(options_);
    }
    return command_channel_->Command(cmd, cb);
}

int RedisClient::CommandArgv(const std::vector <tinynet::string_view>& cmds, RedisCallback cb) {
    if (!command_channel_) {
        command_channel_ = std::make_shared<RedisChannel>(event_loop_);
        command_channel_->Init(options_);
    }
    return command_channel_->CommandArgv(cmds, cb);
}

int RedisClient::Monitor(RedisCallback cb) {
    if (!monitor_channel_) {
        monitor_channel_ = std::make_shared<RedisChannel>(event_loop_);
        monitor_channel_->Init(options_);
    }
    return monitor_channel_->Monitor(cb);
}

int RedisClient::Subscribe(const std::string& name, RedisCallback cb) {
    auto channel = CreateSubscribeChannel(name);
    if (!channel) {
        return tinynet::ERROR_REDIS_SUBSCRIBE;
    }
    return channel->Subscribe(name, cb);
}

int RedisClient::Psubscribe(const std::string& pattern, RedisCallback cb) {
    auto channel = CreateSubscribeChannel(pattern);
    if (!channel) {
        return tinynet::ERROR_REDIS_SUBSCRIBE;
    }
    return channel->Psubscribe(pattern, cb);
}

RedisChannelPtr RedisClient::CreateSubscribeChannel(const std::string& name) {
    auto it = subscribers_.find(name);
    if (it != subscribers_.end()) {
        return it->second;
    }
    auto channel = std::make_shared<RedisChannel>(event_loop_);
    channel->Init(options_);
    subscribers_[name] = channel;
    return channel;
}

RedisChannelPtr RedisClient::GetSubscribeChannel(const std::string& name) {
    auto it = subscribers_.find(name);
    if (it != subscribers_.end()) {
        return it->second;
    }
    return RedisChannelPtr();
}

void RedisClient::Unsubscribe(const std::string& name) {
    auto it = subscribers_.find(name);
    if (it != subscribers_.end()) {
        it->second->Close(ERROR_OK);
        subscribers_.erase(it);
    }
}

void RedisClient::Punsubscribe(const std::string& pattern) {
    Unsubscribe(pattern);
}

}
