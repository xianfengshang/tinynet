// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "stream_socket.h"
#include "base/error_code.h"
#include "util/net_utils.h"
#include "util/string_utils.h"
#include "util/uri_utils.h"


namespace tinynet {
namespace net {
StreamSocket::StreamSocket(EventLoop *loop) :
    Socket(loop) {
}

StreamSocket::StreamSocket(tinynet::EventLoop *loop, int fd, int af, const std::string* peer_address):
    Socket(loop, fd, af, peer_address) {
}

StreamSocket::~StreamSocket() = default;

void StreamSocket::Open() {
    Socket::Open();
    StreamOpen();
}

void StreamSocket::Readable() {
    StreamReadable();
}

void StreamSocket::Writable() {
    if (get_status() == SocketStatus::SS_CONNECTING) {
        if ((mask_ & EVENT_WRITABLE) == 0)
            return;
        int err = NetUtils::GetSockError(fd_) ? ERROR_SOCKET_CONNECTIONREFUSED : ERROR_OK;
        if ((mask_ & EVENT_ERROR) && err == ERROR_OK)
            err = ERROR_SOCKET_CONNECTIONREFUSED;
        if (err != ERROR_OK) {
            SetError(err);
        } else {
            Open();
        }
    } else {
        StreamWritable();
    }

}

void StreamSocket::StreamOpen() {
    Readable();
    Invoke(conn_callback_);
}

}
}

