// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "socket_channel.h"
#include "logging/logging.h"
#include "socket_server.h"
#include "stream_socket.h"
#include "ssl_socket.h"
#include "base/unique_id.h"
#include "base/error_code.h"
#include "util/uri_utils.h"

namespace tinynet {
namespace net {

static int kConnectTimeout = 20000;

SocketChannel::SocketChannel(SocketPtr sock, SocketServer *server) :
    event_loop_(sock->event_loop()),
    guid_(event_loop_->NewUniqueId()),
    server_(server),
    state_(ChannelState::CS_CONNECTED) {
    set_socket(sock);
    address_ = sock->get_peer_address();
    opts_.name = server->get_opts().name;
    opts_.keepalive_ms = server->get_opts().keepalive_ms;
}

SocketChannel::SocketChannel(EventLoop *loop) :
    event_loop_(loop),
    guid_(event_loop_->NewUniqueId()),
    server_(nullptr),
    state_(ChannelState::CS_UNSPEC) {
    set_socket(event_loop_->NewObject<StreamSocket>());
}

SocketChannel::~SocketChannel() = default;

void SocketChannel::Init(ChannelOptions& opts) {
    state_ = ChannelState::CS_UNSPEC;

    opts_ = opts;
    opts_.timeout = opts_.timeout ? opts_.timeout : kConnectTimeout;
    if (opts.name.empty()) opts_.name = "Socket";

    if (!opts_.url.empty()) {
        UriUtils::uri_info info;
        if (UriUtils::parse_uri(opts_.url, info)) {
            opts_.host = info.host;
            opts_.port = info.port;
            opts_.use_ssl = info.scheme == "wss" || info.scheme == "https";
        }
    }
    if (opts_.use_ssl) {
        ssl_ctx_.reset(new(std::nothrow) SSLContext());
        SSLContext::Options ssl_opts;
        ssl_opts.ssl_key = opts_.ssl_key;
        ssl_opts.ssl_cert = opts_.ssl_cert;
        ssl_opts.ssl_ca = opts_.ssl_ca;
        ssl_opts.ssl_capath = opts_.ssl_capath;
        ssl_ctx_->Init(ssl_opts);
    }

    if (!opts_.path.empty()) {
        UriUtils::format_address(address_, "unix", opts_.path, nullptr);
    } else {
        UriUtils::format_address(address_, "tcp", opts_.host, &opts.port);
    }
}

void SocketChannel::HandleConnect() {
    if (opts_.debug) {
        log_info("[%s] %s channel(%lld) connected to server %s success",
                 get_name(), get_name(), guid_, address_.c_str());
    }
    state_ = ChannelState::CS_CONNECTED;
    OnOpen();
}

void SocketChannel::HandleError(int err) {
    if (state_ == ChannelState::CS_CLOSED)
        return;
    state_ = ChannelState::CS_CLOSED;

    OnError(err);

    OnClose();

    if (socket_)
        socket_->Close();

    if (opts_.debug) {
        log_info("[%s] %s channel(%lld, %s) closed, err:%d, msg:%s",
                 get_name(), get_name(), guid_,
                 address_.c_str(), err, tinynet_strerror(err));
    }
    if (server_)
        server_->RemoveChannel(guid_);
}


void SocketChannel::set_socket(SocketPtr sock) {
    if (!(socket_ = std::move(sock)))
        return;
    socket_->set_read_callback(std::bind(&SocketChannel::OnRead, this));
    socket_->set_error_callback(std::bind(&SocketChannel::HandleError, this, std::placeholders::_1));
    socket_->set_conn_callback(std::bind(&SocketChannel::HandleConnect, this));
}

int SocketChannel::Open() {
    SocketPtr sock;
    if (opts_.use_ssl)
        sock = event_loop_->NewObject<SSLSocket>(ssl_ctx_.get());
    else
        sock = event_loop_->NewObject<StreamSocket>();

    if (!sock)
        return ERROR_OS_OOM;

    set_socket(sock);

    int err = opts_.path.empty() ? socket_->Connect(opts_.host, opts_.port, opts_.timeout) : socket_->Connect(opts_.path);
    if (err == ERROR_OK) {
        state_ = ChannelState::CS_CONNECTING;
        if (opts_.debug) {
            log_info("[%s] %s channel(%lld) begin connect to %s server %s",
                     get_name(), get_name(), guid_,
                     get_name(), address_.c_str());
        }
    } else {
        log_info("[%s] %s channel(%lld) begin connect to %s server %s failed, err:%d, msg:%s",
                 get_name(), get_name(), guid_,
                 get_name(), address_.c_str(), err, tinynet_strerror(err));
    }
    return err;
}

void SocketChannel::Close(int err) {
    HandleError(err);
}
}
}
