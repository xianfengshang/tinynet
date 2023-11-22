// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <string>
#include <functional>
#include <queue>
#include "net/event_loop.h"
#include "net/socket_channel.h"
#include "redis_types.h"
#include "redis_codec.h"
#include "base/string_view.h"

using namespace tinynet;

namespace redis {

struct CachedCommand {
    std::vector<std::string> cmds;
    RedisCallback callback;
};

typedef std::shared_ptr<CachedCommand> CachedCommandPtr;

enum ChannelState {
    STATE_UNSPEC = 0,
    STATE_CONNECTING = 1,
    STATE_HANDSHAKE = 2,
    STATE_CONNECTED = 3,
    STATE_CLOSED = 4,
};

enum ChannelMode {
    MODE_REQRES = 0,
    MODE_PUBSUB = 1,
    MODE_MONITOR = 2,
};

/**
 * Asynchronous redis client
 */
class RedisChannel:
    public net::SocketChannel {
  public:
    RedisChannel(tinynet::EventLoop *loop);
    ~RedisChannel();
  private:
    void OnOpen() override;
    void OnError(int err) override;
    void OnRead() override;
    void OnReply(RedisReply* reply);
  public:
    /** Initialize redis client with the given options */
    void Init(const RedisOptions& options);
    /** Send a redis command to server. The given callback will be called when redis server respond for the command. */
    int Command(const tinynet::string_view& cmd, RedisCallback cb);
    int CommandArgv(const std::vector<tinynet::string_view>& cmds, RedisCallback cb);
    int Monitor(RedisCallback cb);
    int Subscribe(const std::string& name, RedisCallback cb);
    int Psubscribe(const std::string& pattern, RedisCallback cb);
  private:
    void Run(int err);
    void Auth();
    void Ping();
    void OnAuth(const RedisReply& reply);
    void OnPong(const RedisReply& reply);
    void OnMonitor(const RedisReply& reply, RedisCallback monitor_cb);
    void OnSubscribe(const RedisReply& reply, RedisCallback subscribe_cb);
    void SendCommands();
    int SendCommand(const tinynet::string_view& cmd, RedisCallback callback);
    int SendCommand(const std::vector<tinynet::string_view>& cmds, RedisCallback callback);
    int SendCommand(const std::vector<std::string>& cmds, RedisCallback callback);
    bool requirepass() { return !options_.password.empty(); }
  private:
    using CommandQueue = std::queue<CachedCommandPtr>;
    using CallbackQueue = std::queue<RedisCallback>;

    CallbackQueue			callback_queue_;
    CommandQueue			request_queue_;
    RedisOptions			options_;
    ChannelMode				mode_;
    std::unique_ptr<RedisCodec>	codec_;
    int64_t					  connect_timer_;
    RedisCallback			push_callback_;
};
}
