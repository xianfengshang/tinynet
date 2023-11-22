#include "http_channel.h"
#include "http_codec.h"
#include "http_server.h"
#include "util/string_utils.h"
#include "util/uri_utils.h"
namespace tinynet {
namespace http {
HttpChannel::HttpChannel(net::SocketPtr sock, net::SocketServer *server) :
    net::SocketChannel(sock, server) {
    auto codec = new(std::nothrow)(HttpCodec);
    codec_.reset(codec);
}

HttpChannel::HttpChannel(EventLoop* loop):
    net::SocketChannel(loop) {
}

HttpChannel::~HttpChannel() = default;

void HttpChannel::OnRead() {
    auto server = get_server<HttpServer>();
    if (!server) {
        return;
    }
    std::vector<std::unique_ptr<HttpMessage> > requests;
    if (!codec_->Read(socket_, &requests)) {
        return;
    }
    for (auto &req : requests) {
        ParseAddress(*req);
        req->guid = get_guid();
        req->remoteAddr = peer_ip_;
        server->HandleRequest(get_guid(), *req);
    }
}

bool HttpChannel::SendResponse(const HttpMessage& resp) {
    if (socket_->is_closed()) {
        return false;
    }
    codec_->Write(socket_, &resp);
    return true;
}

bool HttpChannel::SendResponse(const ZeroCopyHttpMessage& resp) {
    if (socket_->is_closed()) {
        return false;
    }
    codec_->Write(socket_, &resp);
    return true;
}

void HttpChannel::ParseAddress(const tinynet::http::HttpMessage& msg) {
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

}
}
