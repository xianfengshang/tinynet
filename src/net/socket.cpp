// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "socket.h"
#include "base/net_types.h"
#include "util/net_utils.h"
#include "util/uri_utils.h"
#include "event_loop.h"
#include "base/error_code.h"
#include "base/io_buffer_stream.h"

namespace tinynet {
namespace net {

static int kTcpKeepAlive_ms = 60000; //Default keepalive timeout

static int kConnectTimeout_ms = 30000; //Default connect timeout

Socket::Socket(EventLoop* loop):
    FileDescriptor(loop, -1),
    af_(AF_UNSPEC),
    status_(SocketStatus::SS_UNSPEC),
    connect_timer_(INVALID_TIMER_ID) {
}

Socket::Socket(EventLoop* loop, int fd, int af, const std::string* peer_address) :
    FileDescriptor(loop, fd),
    af_(af),
    status_(SocketStatus::SS_UNSPEC),
    connect_timer_(INVALID_TIMER_ID) {
    if (peer_address)
        peer_address_ = *peer_address;
}

Socket::~Socket() {
    Dispose(true);
}

void Socket::Open() {
    status_ = SocketStatus::SS_CONNECTED;
    if (af_ == AF_INET || af_ == AF_INET6) {
        NetUtils::SetNodelay(fd_); //TCP_NODELAY
        NetUtils::SetKeepAlive(fd_, kTcpKeepAlive_ms); //TCP level keep alive
    }

    if ((flag_ & FD_FLAGS_CLIENT_FD) == 0)
        flag_ |= FD_FLAGS_SERVER_FD;

    if (connect_timer_ != INVALID_TIMER_ID) {
        event_loop_->ClearTimer(connect_timer_);
    }
}

void Socket::Close() {
    if (status_ == SocketStatus::SS_CLOSED)
        return;
    FileDescriptor::Close();
    Dispose(false);
}

void Socket::SetError(int err) {
    mask_ |= EVENT_ERROR;
    Invoke(error_callback_, err);
}

size_t Socket::Read(void *buf, size_t len) {
    if (mask_ & EVENT_ERROR)
        return 0;
    return rbuf_.read(buf, len);
}

void Socket::Write(const void *data, size_t len) {
    if (mask_ & EVENT_ERROR)
        return;
    int nwrite = 0;
    if (mask_ & EVENT_WRITABLE) {
        int err = NetUtils::WriteAll(fd_, data, len, &nwrite);
        if (err) {
            SetError(err);
            return;
        }
        if ((size_t)nwrite >= len)
            return;
        mask_ &= ~EVENT_WRITABLE;
    }
    wbuf_.append((const char*)data + nwrite, len - nwrite);
    Writable();
}

void Socket::Flush() {
    if (mask_ & EVENT_ERROR)
        return;
    Writable();
}

int Socket::Connect(const std::string &host, int port, int timeout) {
    int err = 0;
    af_ = AF_INET;
    fd_ = NetUtils::ConnectTcp(host.c_str(), port, af_, &err);
    if (fd_ == -1) {
        return ERROR_SOCKET_BADF;
    }
    if (AddEvent(EVENT_WRITABLE) == -1) {
        err = ERROR_EVENTLOOP_REGISTER;
        return err;
    }
    status_ = SocketStatus::SS_CONNECTING;
    flag_ |= FD_FLAGS_CLIENT_FD;
    UriUtils::format_address(peer_address_, "tcp", host, &port);
    connect_timer_ = event_loop_->AddTimer(timeout > 0 ? timeout : kConnectTimeout_ms, 0,
                                           std::bind(&Socket::SetError, this, ERROR_SOCKET_CONNECTTIMEOUT));
    return err;
}

int Socket::Connect(const std::string& unix_path) {
    int err = 0;
    af_ = AF_UNIX;
    fd_ = NetUtils::ConnectUnix(unix_path.c_str(), &err);
    if (fd_ == -1) {
        return ERROR_SOCKET_BADF;
    }
    if (AddEvent(EVENT_WRITABLE) == -1) {
        err = ERROR_EVENTLOOP_REGISTER;
        return err;
    }
    status_ = SocketStatus::SS_CONNECTING;
    flag_ |= FD_FLAGS_CLIENT_FD;
    UriUtils::format_address(peer_address_, "unix", unix_path, nullptr);
    connect_timer_ = event_loop_->AddTimer(kConnectTimeout_ms, 0,
                                           std::bind(&Socket::SetError, this, ERROR_SOCKET_CONNECTTIMEOUT));
    return err;
}

void Socket::StreamReadable() {
    if (mask_ & EVENT_ERROR)
        return;

    int err = ERROR_OK;
    if (mask_ & EVENT_READABLE) {
        int nbytes = 0;
        for (;;) {
            if (rbuf_.size() == rbuf_.capacity()) {
                rbuf_.reserve(rbuf_.size() + 8196);
            }
            size_t len = rbuf_.capacity() - rbuf_.size();
            int nread = 0;
            err = NetUtils::ReadAll(fd_, rbuf_.end(), len, &nread);
            if (nread > 0) {
                rbuf_.resize(rbuf_.size() + nread);
                nbytes += nread;
            }
            if (err || nread < (int)len) break;
        }

        if (nbytes > 0) {
            Invoke(read_callback_);
        }
        mask_ &= ~EVENT_READABLE;
    }
    if ((mask_ & EVENT_READABLE) == 0 && err == ERROR_OK) {
        if (AddEvent(EVENT_READABLE) == -1) {
            err = ERROR_EVENTLOOP_REGISTER;
        }
    }
    if (err != ERROR_OK) {
        SetError(err);
    }
}

void Socket::StreamWritable() {
    if (mask_ & EVENT_ERROR)
        return;

    int err = ERROR_OK;
    if (mask_ & EVENT_WRITABLE) {
        if (wbuf_.size() > 0) {
            int nbytes = 0;
            err = NetUtils::WriteAll(fd_, wbuf_.begin(), wbuf_.size(), &nbytes);
            if (nbytes > 0) {
                wbuf_.consume(static_cast<size_t>(nbytes));
                Invoke(write_callback_);
            }
        }
        if (wbuf_.size() > 0)
            mask_ &= ~EVENT_WRITABLE;
        else
            ClearEvent(EVENT_WRITABLE);
    }
    if (err == ERROR_OK && wbuf_.size() > 0) {
        if (AddEvent(EVENT_WRITABLE) == -1) {
            err = ERROR_EVENTLOOP_REGISTER;
        }
    }
    if (err != ERROR_OK) {
        SetError(err);
    }
}

void Socket::Dispose(bool disposed) noexcept {
    if (status_ == SocketStatus::SS_CONNECTING) {
        event_loop_->ClearTimer(connect_timer_);
    } else if (status_ == SocketStatus::SS_CLOSED) {
        return;
    }
    status_ = SocketStatus::SS_CLOSED;
    if (disposed) {
        conn_callback_ = nullptr;
    } else {
        rbuf_.clear();
        wbuf_.clear();
    }
}

}
}
