// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <algorithm>
#include <mutex>
#include "listener.h"
#include "logging/logging.h"
#include "base/error_code.h"
#include "base/singleton.h"
#include "util/net_utils.h"
#include "util/uri_utils.h"
#include "util/string_utils.h"

namespace tinynet {

namespace net {


Listener::Listener(EventLoop *loop) :
    Socket(loop),
    listen_port_(0),
    af_(AF_UNSPEC) {
}

Listener::~Listener() = default;

int Listener::BindAndListen(const std::string &ip, int port, int backlog, int flags) {
    int err;
    const char* listen_ip;
    if (ip == "*") { //"*" means it will binds the socket to all available interfaces
        listen_ip = "0.0.0.0";
    } else {
        listen_ip = ip.c_str();
    }
    listen_port_ = port;
    err = 0;
    af_ = AF_INET;
    fd_ = NetUtils::BindAndListenTcp(listen_ip, listen_port_, backlog, flags, &err);
    if (fd_ == -1) {
        return err;
    }
    if (AddEvent(EVENT_READABLE) == -1) {
        err = ERROR_EVENTLOOP_REGISTER;
        return err;
    }
    if (!listen_port_) {
        NetUtils::GetSockName(fd_, nullptr, &listen_port_);
    }
    UriUtils::format_address(listen_address_, "tcp", listen_ip, &listen_port_);
    return err;
}

int Listener::BindAndListen(const std::string &unix_path, int backlog) {
    int err = 0;
#ifdef _WIN32
    af_ = AF_INET;
#else
    af_ = AF_UNIX;
#endif
    fd_ = NetUtils::BindAndListenUnix(unix_path.c_str(), backlog, &err);
    if (fd_ == -1) {
        return err;
    }
    if (AddEvent(EVENT_READABLE) == -1) {
        err = ERROR_EVENTLOOP_REGISTER;
        return err;
    }
    flag_ |= FD_FLAGS_LISTEN_FD;
    UriUtils::format_address(listen_address_, "unix", unix_path, nullptr);
    return err;
}
}
}
