// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "websocket_server.h"
#include "websocket_codec.h"
#include "net/http/http_codec.h"
#include "base/error_code.h"
#include "net/http/http_server.h"
#include "websocket.h"
#include <functional>

namespace tinynet {
namespace websocket {

static const int kUpdateInterval = 1000;

WebSocketServer::WebSocketServer(EventLoop *loop) :
    net::SocketServer(loop),
    websocket_session_callback_(nullptr),
    update_timer_(0) {

}

WebSocketServer::WebSocketServer(EventLoop *loop, SSLContext* ctx) :
    net::SocketServer(loop, ctx),
    websocket_session_callback_(nullptr),
    update_timer_(0) {
}


WebSocketServer::~WebSocketServer() {
    if (update_timer_) {
        event_loop_->ClearTimer(update_timer_);
    }
}

int WebSocketServer::Start(net::ServerOptions& opts) {
    int err;
    if (opts.name.empty()) opts.name = "WebSocket";

    if ((err = net::SocketServer::Start(opts)) == ERROR_OK) {
        update_timer_ = event_loop_->AddTimer(kUpdateInterval, kUpdateInterval,
                                              std::bind(&WebSocketServer::Update, this));
    }
    return err;
}

void WebSocketServer::Stop() {
    if (update_timer_) {
        event_loop_->ClearTimer(update_timer_);
    }
    SocketServer::Stop();
}

net::SocketChannelPtr WebSocketServer::CreateChannel(net::SocketPtr sock) {
    auto session =  std::make_shared<WebSocket>(sock, this);
    auto onopen = std::bind(&WebSocketServer::websocket_session_onopen, this, session->get_guid());
    session->set_onopen_callback(onopen);
    auto onmessage = std::bind(&WebSocketServer::websocket_session_onmessage, this, session->get_guid(), std::placeholders::_1);
    session->set_onmessage_callback(onmessage);
    auto onerror = std::bind(&WebSocketServer::websocket_session_onerror, this, session->get_guid(), std::placeholders::_1);
    session->set_onerror_callback(onerror);
    auto onclose = std::bind(&WebSocketServer::websocket_session_onclose, this, session->get_guid());
    session->set_onclose_callback(onclose);
    return session;
}

void WebSocketServer::CloseSession(int64_t session_guid) {
    auto session = GetChannel<WebSocket>(session_guid);
    if (session) {
        session->Close(ERROR_WEBSOCKET_CLOSEDBYSERVER);
    }
}

bool WebSocketServer::Send(int64_t channel_guid, const WebSocketMessage& msg ) {
    auto session = GetChannel<WebSocket>(channel_guid);
    return session ? session->Send(msg) : false;
}


void WebSocketServer::Broadcast(const WebSocketMessage& msg) {
    for (auto& entry : channels_) {
        auto session = std::static_pointer_cast<WebSocket>(entry.second);
        if (session) {
            session->Send(msg);
        }
    }
}

void WebSocketServer::Update() {
    std::vector<WebSocketPtr> zombie_sessions;
    for (auto it : channels_) {
        auto session = std::static_pointer_cast<WebSocket>(it.second);
        if (session && !session->is_alive()) {
            zombie_sessions.push_back(session);
        }
    }
    for (auto session : zombie_sessions) {
        session->Close(ERROR_WEBSOCKET_KEEPALIVETIMEOUT);
    }
}

void WebSocketServer::websocket_session_onopen(int64_t session_guid) {
    if (websocket_session_callback_ != nullptr) {
        server::WebSocketSessionEvent evt;
        evt.type = server::WEBSOCKET_SESSION_EVENT_ON_OPEN;
        evt.guid = session_guid;
        websocket_session_callback_(evt);
    }
}

void WebSocketServer::websocket_session_onclose(int64_t session_guid) {
    if (websocket_session_callback_ != nullptr) {
        server::WebSocketSessionEvent evt;
        evt.type = server::WEBSOCKET_SESSION_EVENT_ON_CLOSE;
        evt.guid = session_guid;
        websocket_session_callback_(evt);
    }
}

void WebSocketServer::websocket_session_onmessage(int64_t session_guid, const WebSocketMessage* msg) {
    if (websocket_session_callback_ != nullptr) {
        server::WebSocketSessionEvent evt;
        evt.type = server::WEBSOCKET_SESSION_EVENT_ON_MESSAGE;
        evt.guid = session_guid;
        evt.msg = msg;
        websocket_session_callback_(evt);
    }
}

void WebSocketServer::websocket_session_onerror(int64_t session_guid, int err) {
    if (websocket_session_callback_ != nullptr) {
        server::WebSocketSessionEvent evt;
        evt.type = server::WEBSOCKET_SESSION_EVENT_ON_ERROR;
        evt.guid = session_guid;
        evt.err = err;
        websocket_session_callback_(evt);
    }
}

}
}
