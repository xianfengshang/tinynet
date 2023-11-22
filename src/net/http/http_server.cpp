// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "http_server.h"
#include "http_channel.h"
#include "util/string_utils.h"
#include "util/uri_utils.h"
#include "base/error_code.h"
#include "logging/logging.h"
namespace tinynet {
namespace http {

HttpServer::HttpServer(EventLoop *loop) :
    net::SocketServer(loop) {

}

HttpServer::~HttpServer() = default;

net::SocketChannelPtr HttpServer::CreateChannel(net::SocketPtr sock) {
    return std::make_shared<HttpChannel>(sock, this);
}

int HttpServer::Start(net::ServerOptions& opts) {
    if (opts.name.empty()) opts.name = "Http";
    return SocketServer::Start(opts);
}

bool HttpServer::SendResponse(const HttpMessage& resp ) {
    auto session = std::static_pointer_cast<HttpChannel>(GetChannel(resp.guid));
    if (session) {
        return session->SendResponse(resp);
    }
    return false;
}

bool HttpServer::SendResponse(const ZeroCopyHttpMessage& resp) {
    auto session = std::static_pointer_cast<HttpChannel>(GetChannel(resp.guid));
    if (session) {
        return session->SendResponse(resp);
    }
    return false;
}

void HttpServer::HandleRequest(int64_t session_guid, const HttpMessage& request ) {
    Invoke(http_callback_, request);
}
}
}
