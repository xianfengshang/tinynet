// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "socket_server.h"
#include "stream_listener.h"
#include "ssl_listener.h"
#include "socket_channel.h"
#include "logging/logging.h"
#include "base/error_code.h"
#include "util/net_utils.h"
#include "util/uri_utils.h"

namespace tinynet {
namespace net {

SocketServer::SocketServer(EventLoop *loop) :
    event_loop_(loop),
    removing_task_(INVALID_TASK_ID),
    ssl_ctx_(nullptr) {
}

SocketServer::SocketServer(EventLoop *loop, SSLContext* ctx) :
    event_loop_(loop),
    removing_task_(INVALID_TASK_ID),
    ssl_ctx_(ctx) {
}

SocketServer::~SocketServer() {
    if (removing_task_) {
        event_loop_->CancelTask(removing_task_);
    }
}

int SocketServer::Start(ServerOptions& opts) {
    if (listener_)
        return ERROR_SERVER_STARTED;

    opts_ = opts;

    if (opts_.name.empty())
        opts_.name = "socket";

    auto listener = CreateListener();
    if (!listener)
        return ERROR_OS_OOM;

    int err = ERROR_INVAL;
    int flags = 0;
    if (opts_.reuseport) flags |= TCP_FLAGS_REUSEPORT;
    if (opts_.ipv6only) flags |= TCP_FLAGS_IPV6ONLY;

    if (!opts_.listen_path.empty()) {
        err = listener->BindAndListen(opts_.listen_path, ksomaxconn);
        goto FINAL;
    }
    if (!opts_.listen_url.empty()) {
        if (!UriUtils::parse_address(opts_.listen_url, &opts_.listen_ip, &opts_.listen_port)) {
            err = ERROR_URI_UNRECOGNIZED;
            goto FINAL;
        }
    }
    if (opts_.listen_port != -1) {
        err = listener->BindAndListen(opts_.listen_ip, opts_.listen_port, ksomaxconn, flags);
        goto FINAL;
    }
    for (int port = std::get<0>(opts_.listen_ports); port < std::get<1>(opts_.listen_ports); ++port) {
        if ((err = listener->BindAndListen(opts_.listen_ip, port, net::ksomaxconn, flags)) == ERROR_OK)
            goto FINAL;
    }
FINAL:
    if (err == ERROR_OK) {
        set_listener(listener);
    }
    return err;
}

void SocketServer::Stop() {
    if (removing_task_) {
        event_loop_->CancelTask(removing_task_);
    }
    if (listener_) {
        listener_->Close();
    }
    channels_.clear();
}

void SocketServer::HandleAccept(SocketPtr sock) {
    auto channel = CreateChannel(sock);
    channels_[channel->get_guid()] = channel;
    if (opts_.debug) {
        log_info("[%s] %s server accept new channel guid(%lld, %s)",
                 get_name(), get_name(), channel->get_guid(), channel->get_address().c_str());
    }
}

void SocketServer::HandleError(int err) {
    log_error("[%s] %s server listening on %s closed, err:%d, msg:%s",
              get_name(), get_name(),
              listener_->get_listen_address().c_str(), err, tinynet_strerror(err));
}

void SocketServer::RemoveChannel(ChannelID guid) {
    auto it = channels_.find(guid);
    if (it == channels_.end()) {
        return;
    }
    removing_channels_.insert(guid);
    if (removing_task_ == INVALID_TASK_ID) {
        removing_task_ = event_loop_->AddTask(std::bind(&SocketServer::RemoveChannels, this));
    }
    if (opts_.debug) {
        auto& channel = it->second;
        log_info("[%s] %s server remove channel(%lld, %s)",
                 get_name(), get_name(), channel->get_guid(),channel->get_address().c_str());
    }

}

SocketChannelPtr SocketServer::GetChannel(ChannelID guid) {
    auto it = channels_.find(guid);
    if (it == channels_.end()) {
        return nullptr;
    }
    return it->second;
}

void SocketServer::RemoveChannels() {
    removing_task_ = INVALID_TASK_ID;
    if (removing_channels_.empty()) return;
    CHANNEL_ID_SET remove_channels;
    removing_channels_.swap(remove_channels);
    for (auto& guid : remove_channels) {
        channels_.erase(guid);
    }
}

void SocketServer::set_listener(net::ListenerPtr listener) {
    if (!(listener_ = std::move(listener))) return;

    listener_->set_conn_callback(std::bind(&SocketServer::HandleAccept, this, std::placeholders::_1));
    listener_->set_error_callback(std::bind(&SocketServer::HandleError, this, std::placeholders::_1));
}

ListenerPtr SocketServer::CreateListener() {
    if (ssl_ctx_)
        return event_loop()->NewObject<SSListener>(ssl_ctx_);
    else
        return event_loop()->NewObject<StreamListener>();
}
}
}
