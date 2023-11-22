// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "websocket.h"
#include "net/http/http_codec.h"
#include "websocket_codec.h"
#include "websocket_server.h"
#include "base/error_code.h"
#include "base/crypto.h"
#include "util/random_utils.h"
#include "util/string_utils.h"
#include "util/uri_utils.h"
#include "logging/logging.h"

namespace tinynet {
namespace websocket {

static const char WEBSOCKET_MAGIC[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static const int kUpdateInterval = 1000;

WebSocket::WebSocket(net::SocketPtr sock, WebSocketServer* server):
    net::SocketChannel(sock, server),
    handshake_(false),
    keepalive_time_(0),
    update_timer_(INVALID_TIMER_ID),
    ping_time_(0) {
    codec_http_.reset(new (std::nothrow)tinynet::http::HttpCodec());
    codec_websocket_.reset(new (std::nothrow) WebSocketCodec(WebSocketCodec::CodecSide::SERVER));
    if (server) {
        opts_.keepalive_ms = server->get_keepalive_ms();
        codec_websocket_->set_max_packet_size(server->get_max_packet_size());
    }
}

WebSocket::WebSocket(EventLoop *loop) :
    net::SocketChannel(loop),
    handshake_(false),
    keepalive_time_(0),
    update_timer_(INVALID_TIMER_ID),
    ping_time_(0) {
    codec_http_.reset(new (std::nothrow)tinynet::http::HttpCodec());
    codec_websocket_.reset(new (std::nothrow) WebSocketCodec(WebSocketCodec::CodecSide::CLIENT));
}


WebSocket::~WebSocket() {
    Dispose(true);
}

void WebSocket::Dispose(bool disposed) {
    handshake_ = false;
    if (update_timer_) {
        event_loop_->ClearTimer(update_timer_);
    }

    if (disposed) {
        onopen_callback_ = nullptr;
        onmessage_callback_ = nullptr;
        onerror_callback_ = nullptr;
        onclose_callback_ = nullptr;
    }
}

void WebSocket::OnOpen() {
    Handshake();
}

void WebSocket::OnRead() {
    if (!handshake_) {
        std::vector<std::unique_ptr<tinynet::http::HttpMessage> > messages;
        if (!codec_http_->Read(socket_, &messages)) {
            return;
        }
        if (messages.empty()) {
            Close(ERROR_WEBSOCKET_HANDSHAKE);
            return;
        }
        auto &msg = messages.front();
        ParseAddress(*msg);
        HandleHandshake(*msg);
    }
    WebSocketMessage* msg;
    while (socket_->is_connected() && (msg = codec_websocket_->Read(socket_)) != nullptr) {
        HandleMessage(msg);
    }

}

void WebSocket::OnError(int err) {
    log_info("WebSocket(%lld,%s) closed, err:%d, msg:%s",
             guid_, peer_ip_.c_str(), err, tinynet_strerror(err));

    Invoke(onerror_callback_, err);
}

void WebSocket::OnClose() {
    Invoke(onclose_callback_);
    Dispose(false);
}

void WebSocket::Update() {
    if (!is_alive()) {
        int err = handshake_ ? ERROR_WEBSOCKET_KEEPALIVETIMEOUT : ERROR_WEBSOCKET_HANDSHAKETIMEOUT;
        Close(err);
        return;
    }
    if (handshake_ && get_ping_interval() > 0) {
        if (event_loop_->Time() >= ping_time_ + get_ping_interval()) {
            ping_time_ = event_loop_->Time();
            Ping();
        }
    }

}

void WebSocket::Handshake() {
    std::string path;
    UriUtils::parse_path(url_, &path);
    if (path.empty()) {
        path = "/";
    }
    sec_ws_key_ = Crypto::base64_encode(std::to_string(RandomUtils::Random64()));
    tinynet::http::HttpMessage req;
    req.type = tinynet::http::HTTP_REQUEST;
    req.method = "GET";
    req.path = path;
    req.headers["HOST"] = peer_ip_;
    req.headers["Connection"] = "Upgrade";
    req.headers["Upgrade"] = "websocket";
    req.headers["Sec-WebSocket-Version"] = std::to_string(WS_Rfc6455);
    req.headers["Sec-WebSocket-Key"] = sec_ws_key_;
    req.headers["Origin"] = url_;
    req.headers["User-Agent"] = "TinynetRuntime/1.0.0";
    codec_http_->Write(socket_, &req);

    ping_time_ = 0;
    if (update_timer_) {
        event_loop_->ClearTimer(update_timer_);
    }
    update_timer_ = event_loop_->AddTimer(kUpdateInterval, kUpdateInterval, std::bind(&WebSocket::Update, this));
}

void WebSocket::HandleHandshake(const tinynet::http::HttpMessage& msg) {
    if (server_) {
        OnHandshakeReq(msg);
    } else {
        OnHandshakeAck(msg);
    }
    if (!handshake_) return;

    keepalive_time_ = event_loop_->Time();
    log_info("WebSocket(%lld, %s) handshake successfully", guid_, peer_ip_.c_str());
    Invoke(onopen_callback_);
}

void WebSocket::OnHandshakeReq(const tinynet::http::HttpMessage& msg) {
    auto it = msg.headers.find("upgrade");
    if (it == msg.headers.end() || it->second != "websocket") {
        Close(ERROR_WEBSOCKET_HANDSHAKE);
        return;
    }
    it = msg.headers.find("sec-websocket-key");
    if (it == msg.headers.end()) {
        Close(ERROR_WEBSOCKET_HANDSHAKE);
        return;
    }
    sec_ws_key_ = it->second;
    tinynet::http::HttpMessage res;
    res.type = tinynet::http::HTTP_RESPONSE;
    res.statusCode = HTTP_STATUS_SWITCHING_PROTOCOLS;
    res.headers["Connection"] = "upgrade";
    res.headers["Upgrade"] = "websocket";
    res.headers["Sec-WebSocket-Accept"] = EncryptKey(sec_ws_key_);
    codec_http_->Write(socket_, &res);

    handshake_ = true;
}

void WebSocket::OnHandshakeAck(const tinynet::http::HttpMessage& msg) {
    auto it = msg.headers.find("upgrade");
    if (it == msg.headers.end() || it->second != "websocket") {
        Close(ERROR_WEBSOCKET_HANDSHAKE);
        return;
    }
    it = msg.headers.find("sec-websocket-accept");
    if (it == msg.headers.end()) {
        Close(ERROR_WEBSOCKET_HANDSHAKE);
        return;
    }
    std::string sec_ws_accept = EncryptKey(sec_ws_key_);
    if (sec_ws_accept != it->second) {
        Close(ERROR_WEBSOCKET_HANDSHAKE);
        return;
    }
    handshake_ = true;
}

void WebSocket::HandleMessage(const WebSocketMessage* msg) {
    auto self = std::static_pointer_cast<WebSocket>(shared_from_this());
    switch (msg->opcode) {
    case Opcode::Continue: {
        break;
    }
    case Opcode::Text:
    case Opcode::Binary: {
        if (self->onmessage_callback_) {
            self->onmessage_callback_(msg);
        }
        break;
    }
    case Opcode::Close: {
        self->Close(ERROR_WEBSOCKET_CLOSEDBYPEER);
        break;
    }
    case Opcode::Ping: {
        self->Pong(msg->data);
        break;
    }
    case Opcode::Pong: {
        break;
    }
    default:
        break;
    }
    self->keepalive_time_ = event_loop_->Time();
}

bool WebSocket::Send(const WebSocketMessage& msg) {
    if (socket_ && socket_->is_connected()) {
        codec_websocket_->Write(socket_, &msg);
        return true;
    }
    return false;
}

bool WebSocket::Open(net::ChannelOptions& opts) {
    switch (get_state()) {
    case net::ChannelState::CS_CONNECTING:
    case net::ChannelState::CS_CONNECTED:
        return false;
    default:
        break;
    }
    if (opts.name.empty()) opts.name = "WebSocket";

    SocketChannel::Init(opts);

    peer_ip_ = opts_.host;
    url_ = opts_.url;
    return SocketChannel::Open() == ERROR_OK;
}

bool WebSocket::Open(const std::string& url) {
    net::ChannelOptions opts;
    opts.url = url;
    return Open(opts);
}

void WebSocket::Ping() {
    WebSocketMessage ping;
    ping.opcode = Opcode::Ping;
    Send(ping);
}

void WebSocket::Pong(const std::string& data) {
    WebSocketMessage pong;
    pong.opcode = Opcode::Pong;
    pong.data = data;
    Send(pong);
}

void WebSocket::ParseAddress(const tinynet::http::HttpMessage& msg) {
    auto it = msg.headers.find("x-real-ip");
    if (it != msg.headers.end()) {
        peer_ip_ = it->second;
        return;
    }
    it = msg.headers.find("x-forwarded-for");
    if (it != msg.headers.end()) {
        std::vector<std::string> ip_vec;
        StringUtils::Split(it->second, ",", ip_vec);
        if (!ip_vec.empty()) {
            peer_ip_ = ip_vec[0];
            return;
        }
    }
    UriUtils::parse_address(socket_->get_peer_address(), &peer_ip_, nullptr);
}

std::string WebSocket::EncryptKey(const std::string& key) {
    std::string buffer;
    buffer.append(key);
    buffer.append(WEBSOCKET_MAGIC);
    std::string sha1_result = Crypto::sha1(buffer);
    return Crypto::base64_encode(sha1_result);
}

bool WebSocket::is_alive() {
    if (get_keepalive_ms() > 0) {
        if (keepalive_time_ == 0) {
            keepalive_time_ = event_loop_->Time();
            return true;
        }
        return event_loop_->Time() < keepalive_time_ + get_keepalive_ms();
    }
    return true;
}
}
}
