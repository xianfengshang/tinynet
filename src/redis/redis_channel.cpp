// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "redis_channel.h"
#include "logging/logging.h"
#include "base/error_code.h"
#include "util/net_utils.h"
#include "base/net_types.h"
namespace redis {

static const char* REDIS_COMMAND_PING = "ping";
static const char* REDIS_COMMAND_AUTH = "auth";
static const char* REDIS_COMMAND_MONITOR = "monitor";
static const char* REDIS_COMMAND_SUBSCRIBE = "subscribe";
//static const char* REDIS_COMMAND_PSUBSCRIBE = "psubscribe";

RedisChannel::RedisChannel(tinynet::EventLoop *loop) :
    SocketChannel(loop),
    mode_(MODE_REQRES),
    codec_(new (std::nothrow) RedisCodec()),
    connect_timer_(INVALID_TIMER_ID) {
}

RedisChannel::~RedisChannel() {
    if (connect_timer_)
        event_loop_->ClearTimer(connect_timer_);
}

void RedisChannel::Init(const RedisOptions& options) {
    options_ = options;
    options_.timeout = options_.timeout < kDefaultConnectTimeout ? kDefaultConnectTimeout : options_.timeout;

    net::ChannelOptions opts;
    opts.name = "Redis";
    opts.timeout = options_.timeout;
    opts.path = options.path;
    opts.host = options.host;
    opts.port = options.port;
    net::SocketChannel::Init(opts);
}

void RedisChannel::OnOpen() {
    connect_timer_ = event_loop_->AddTimer(options_.timeout, 0,
                                           std::bind(&RedisChannel::Close, this, tinynet::ERROR_REDIS_HANDSHAKE));
    if (requirepass()) {
        Auth();
    } else {
        Ping();
    }
}

void RedisChannel::OnError(int err) {
    if (get_state() == net::ChannelState::CS_CONNECTING) {
        log_error("[Redis] Redis channel(%lld, %s:%d) connect to redis server failed, err:%s",
                  guid_, options_.host.c_str(), options_.port, tinynet_strerror(err));
    } else {
        log_error("[Redis] Redis channel(%lld, %s:%d) lost connection to server, err:%s",
                  guid_, options_.host.c_str(), options_.port, tinynet_strerror(err));
    }
    event_loop_->ClearTimer(connect_timer_);

    Run(err);
}

void RedisChannel::OnRead() {
    RedisReply* reply;
    while (socket_->is_connected() && (reply = codec_->Read(socket_)) != nullptr) {
        OnReply(reply);
    }

}

void RedisChannel::OnReply(RedisReply* reply) {
    switch (mode_) {
    case MODE_REQRES: {
        if (callback_queue_.empty()) {
            log_error("[Redis] Redis channel(%lld, %s:%d) sequence error", guid_, options_.host.c_str(), options_.port);
            return;
        }
        auto callback = callback_queue_.front();
        callback_queue_.pop();
        callback(*reply);
        break;
    }
    case MODE_PUBSUB:
    case MODE_MONITOR: {
        if (push_callback_) {
            push_callback_(*reply);
        }
        break;
    }
    default:
        break;
    }

}

int RedisChannel::Command(const tinynet::string_view& cmd, RedisCallback cb) {
    int err = tinynet::ERROR_OK;
    switch (get_state()) {
    case net::ChannelState::CS_CLOSED:
    case net::ChannelState::CS_UNSPEC: //BeginConnect => Enqueue
        if ((err = Open())) break;
    case net::ChannelState::CS_CONNECTING: {
        auto req = std::make_shared<CachedCommand>();
        req->callback = std::move(cb);
        req->cmds.emplace_back(cmd.data(), cmd.size());
        request_queue_.emplace(std::move(req));
        break;
    }
    case net::ChannelState::CS_CONNECTED: {
        err = SendCommand(cmd, std::move(cb));
        break;
    }
    default:
        err = ERROR_FAILED;
        break;
    }
    return err;
}

int RedisChannel::CommandArgv(const std::vector<tinynet::string_view> &iovs, RedisCallback cb) {
    int err = tinynet::ERROR_OK;
    switch (get_state()) {
    case net::ChannelState::CS_CLOSED:
    case net::ChannelState::CS_UNSPEC: //BeginConnect => Enqueue
        if ((err = Open())) break;
    case net::ChannelState::CS_CONNECTING: {
        auto req = std::make_shared<CachedCommand>();
        req->callback = std::move(cb);
        for (auto& iov : iovs) {
            req->cmds.emplace_back(iov.data(), iov.size());
        }
        request_queue_.emplace(std::move(req));
        break;
    }
    case net::ChannelState::CS_CONNECTED: {
        err = SendCommand(iovs, cb);
        break;
    }
    default:
        err = ERROR_FAILED;
        break;
    }
    return err;
}

int RedisChannel::Monitor(RedisCallback cb) {
    tinynet::string_view cmd(REDIS_COMMAND_MONITOR);
    return Command(cmd, std::bind(&RedisChannel::OnMonitor, this, std::placeholders::_1, cb));
}

int RedisChannel::Subscribe(const std::string& name, RedisCallback cb) {
    std::vector<tinynet::string_view> cmds;
    cmds.emplace_back(REDIS_COMMAND_SUBSCRIBE);
    cmds.emplace_back(name);
    return CommandArgv(cmds, std::bind(&RedisChannel::OnSubscribe, this, std::placeholders::_1, cb));
}

int RedisChannel::Psubscribe(const std::string& pattern, RedisCallback cb) {
    std::vector<tinynet::string_view> cmds;
    cmds.emplace_back(REDIS_COMMAND_SUBSCRIBE);
    cmds.emplace_back(pattern);
    return CommandArgv(cmds, std::bind(&RedisChannel::OnSubscribe, this, std::placeholders::_1, cb));
}

void RedisChannel::Run(int err) {
    RedisReply reply;
    reply.type = REDIS_REPLY_ERROR;
    reply.str = tinynet_strerror(err);
    reply.integer = 0;

    if (request_queue_.size() > 0) {
        CommandQueue request_queue;
        request_queue_.swap(request_queue);
        while (request_queue.size() > 0) {
            auto req = request_queue.front();
            request_queue.pop();
            req->callback(reply);
        }
    }
    if (request_queue_.size() > 0) {
        CommandQueue pending_queue;
        request_queue_.swap(pending_queue);
        while (pending_queue.size() > 0) {
            auto req = pending_queue.front();
            pending_queue.pop();
            req->callback(reply);
        }
    }

}

void RedisChannel::Ping() {
    tinynet::string_view cmd(REDIS_COMMAND_PING);
    SendCommand(cmd, std::bind(&RedisChannel::OnPong, this, std::placeholders::_1));
}

void RedisChannel::Auth() {
    std::vector<std::string> cmds;
    cmds.emplace_back(REDIS_COMMAND_AUTH);
    cmds.emplace_back(options_.password);
    SendCommand(cmds, std::bind(&RedisChannel::OnAuth, this, std::placeholders::_1));
}

void RedisChannel::OnAuth(const RedisReply& reply) {
    if (reply.type == REDIS_REPLY_ERROR) {
        log_warning("Redis server %s:%d respond error:%.*s for auth request.",
                    options_.host.c_str(), options_.port, (int)reply.str.size(), reply.str.data());
        Close(ERROR_REDIS_HANDSHAKE);
        return;
    } else {
        log_info("Redis server %s:%d  respond %.*s for auth request.",
                 options_.host.c_str(), options_.port, (int)reply.str.size(), reply.str.data());
    }

    event_loop_->ClearTimer(connect_timer_);

    // Check pending request
    SendCommands();
}

void RedisChannel::OnPong(const RedisReply& reply) {
    if (reply.type == REDIS_REPLY_ERROR) {
        log_warning("Redis server %s %d respond error:%.*s for ping request.",
                    options_.host.c_str(), options_.port, (int)reply.str.size(), reply.str.data());
        Close(ERROR_REDIS_HANDSHAKE);
        return;
    } else {
        log_info("Redis server %s %d respond %.*s for ping request.",
                 options_.host.c_str(), options_.port, (int)reply.str.size(), reply.str.data());
    }
    if (connect_timer_)
        event_loop_->ClearTimer(connect_timer_);

    // Check pending request
    SendCommands();
}

void RedisChannel::OnMonitor(const RedisReply& reply, RedisCallback monitor_cb) {
    if (reply.type == REDIS_REPLY_ERROR) {
        log_error("[Redis] Redis channel(%lld,%s:%d) enter monitor mode failed, err:%.*s",
                  guid_, options_.host.c_str(), options_.port, (int)reply.str.size(), reply.str.data());
        return;
    } else {
        log_info("[Redis] Redis channel(%lld,%s:%d) enter monitor mode",
                 guid_, options_.host.c_str(), options_.port);
    }
    mode_ = MODE_MONITOR;
    push_callback_ = monitor_cb;
}

void RedisChannel::OnSubscribe(const RedisReply& reply, RedisCallback subscribe_cb) {
    if (reply.type == REDIS_REPLY_ERROR) {
        log_error("[Redis] Redis channel(%lld,%s:%d) enter pub/sub mode failed, err:%.*s",
                  guid_, options_.host.c_str(), options_.port, (int)reply.str.size(), reply.str.data());
        return;
    } else {
        log_info("[Redis] Redis channel(%lld,%s:%d) enter pub/sub mode",
                 guid_, options_.host.c_str(), options_.port);
    }
    mode_ = MODE_PUBSUB;
    push_callback_ = subscribe_cb;
}

void RedisChannel::SendCommands() {
    while (request_queue_.size() > 0) {
        auto req = request_queue_.front();
        request_queue_.pop();
        SendCommand(req->cmds, req->callback);
    }
}

int RedisChannel::SendCommand(const tinynet::string_view& cmd, RedisCallback callback) {
    if (socket_ && socket_->is_connected()) {
        codec_->Write(socket_, cmd);
        callback_queue_.emplace(std::move(callback));
        return 0;
    }
    return tinynet::ERROR_REDIS_CONNECTIONCLOSED;
}

int RedisChannel::SendCommand(const std::vector<tinynet::string_view>& iovs, RedisCallback callback) {
    if (socket_ && socket_->is_connected()) {
        if (iovs.size() == 1) {
            codec_->Write(socket_, iovs[0]);
        } else {
            codec_->Write(socket_, iovs);
        }
        callback_queue_.emplace(std::move(callback));
        return 0;
    }
    return tinynet::ERROR_REDIS_CONNECTIONCLOSED;
}

int RedisChannel::SendCommand(const std::vector<std::string>& cmds, RedisCallback callback) {
    std::vector<tinynet::string_view> iovs;
    for (auto& cmd : cmds) {
        iovs.emplace_back(cmd);
    }
    return SendCommand(iovs, callback);
}

}
