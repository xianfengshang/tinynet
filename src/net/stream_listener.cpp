// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <algorithm>
#include <mutex>
#include "stream_listener.h"
#include "stream_socket.h"
#include "logging/logging.h"
#include "base/error_code.h"
#include "base/singleton.h"
#include "util/net_utils.h"
#include "util/uri_utils.h"
#include "util/string_utils.h"

namespace tinynet {

namespace net {


StreamListener::StreamListener(EventLoop *loop) :
    Listener(loop) {
}

StreamListener::~StreamListener() = default;

void StreamListener::Readable() {
    std::string peer_ip, peer_address;
    int peer_port, err;
    err = 0;
    for (;;) {
        peer_ip.clear();
        peer_address.clear();
        int accept_fd = NetUtils::Accept(fd_, &peer_ip, &peer_port, &err);
        if (accept_fd == -1) {
            break;
        }
        UriUtils::format_address(peer_address, af_ == AF_UNIX ? "unix" : "tcp",
                                 peer_ip, af_ == AF_UNIX ? nullptr : &peer_port);

        auto sock = event_loop_->NewObject<StreamSocket>(accept_fd, af_, &peer_address);
        sock->Open();
        Invoke(conn_callback_, sock);
    }
    if (err) {
        log_warning("StreamListener(%s) accepting incoming connection error, err:%d, msg:%s",
                    listen_address_.c_str(), err, tinynet_strerror(err));
    }
    if (AddEvent(EVENT_READABLE) == -1) {
        SetError(ERROR_EVENTLOOP_REGISTER);
    }
}
}
}
