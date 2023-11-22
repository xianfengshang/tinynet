// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "ssl_socket.h"
#include "base/error_code.h"
#include "util/net_utils.h"
#include "util/string_utils.h"
#include "util/uri_utils.h"


namespace tinynet {
namespace net {

SSLSocket::SSLSocket(EventLoop *loop, SSLContext *ctx) :
    Socket(loop),
    ssl_ctx_(ctx),
    handshake_status_(HS_NONE),
    ssl_(nullptr) {
}


SSLSocket::SSLSocket(tinynet::EventLoop *loop, tinynet::SSLContext *ctx, int fd, int af, const std::string* peer_address) :
    Socket(loop, fd, af, peer_address),
    ssl_ctx_(ctx),
    handshake_status_(HS_NONE),
    ssl_(nullptr) {
}

SSLSocket::~SSLSocket() {
    SSLClose();
}

void SSLSocket::Open() {
    Socket::Open();
    SSLOpen();
}

void SSLSocket::Close() {
    Socket::Close();
    SSLClose();
}

void SSLSocket::Readable() {
    if (handshake_status_ == HS_NONE) {
        StreamReadable();
    } else if (handshake_status_ == HS_HANDHAKING) {
        SSLHandshake();
    } else if(handshake_status_ == HS_HANDSHAKED) {
        SSLReadable();
    }
}

void SSLSocket::Writable() {
    if (status_ == SocketStatus::SS_CONNECTING) {
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
    } else if (status_ == SocketStatus::SS_CONNECTED) {
        if (handshake_status_ == HS_NONE) {
            StreamWritable();
        } else if (handshake_status_ == HS_HANDHAKING) {
            SSLHandshake();
        } else if(handshake_status_ == HS_HANDSHAKED) {
            SSLWritable();
        }
    }
}
void SSLSocket::SSLOpen() {
    Readable();
    Invoke(conn_callback_);
}

void SSLSocket::Handshake() {
    if (!ssl_ctx_)
        return;

    handshake_status_ = HS_HANDHAKING;
    if (ssl_) {
        SSL_free(ssl_);
    }
    ssl_ = SSL_new(ssl_ctx_->get_ctx());
    SSL_set_fd(ssl_, fd_);
    if (flag_ & FD_FLAGS_CLIENT_FD) {
        SSL_set_connect_state(ssl_);
    } else {
        SSL_set_accept_state(ssl_);
    }
    SSLHandshake();

}

void SSLSocket::SSLClose() {
    if (ssl_ != nullptr) {
        SSL_CTX_remove_session(ssl_ctx_->get_ctx(), SSL_get0_session(ssl_));
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
}

void SSLSocket::SSLReadable() {
    if (mask_ & EVENT_ERROR)
        return;

    int err = ERROR_OK;
    int mask = EVENT_NONE;
    if (mask_ & EVENT_READABLE) {
        int nbytes = 0;
        for (;;) {
            if (rbuf_.size() == rbuf_.capacity()) {
                rbuf_.reserve(rbuf_.size() + 8192);
            }
            int len = (int)(rbuf_.capacity() - rbuf_.size());
            int nread = SSL_read(ssl_, rbuf_.end(), len);
            if (nread > 0) {
                rbuf_.resize(rbuf_.size() + nread);
                nbytes += nread;
            } else {
                int ssl_err = SSL_get_error(ssl_, nread);
                if (ssl_err == SSL_ERROR_WANT_READ) {
                    mask |= EVENT_READABLE;
                } else if (ssl_err == SSL_ERROR_WANT_WRITE) {
                    mask |= EVENT_WRITABLE;
                } else {
                    err = -1;
                }
                break;
            }
        }

        if (nbytes > 0) {
            Invoke(read_callback_);
        }
        mask_ &= ~EVENT_READABLE;
    }
    if (err == ERROR_OK && mask != EVENT_NONE) {
        if (AddEvent(mask) == -1) {
            err = ERROR_EVENTLOOP_REGISTER;
        }
    }
    if (err != ERROR_OK) {
        SetError(err);
    }
}

void SSLSocket::SSLWritable() {
    if (mask_ & EVENT_ERROR)
        return;

    int err = ERROR_OK;
    int mask = EVENT_NONE;
    if (mask_ & EVENT_WRITABLE) {
        int nbytes = 0;
        for (;;) {
            if (wbuf_.size() > 0) {
                int nwrite = SSL_write(ssl_, wbuf_.begin(), (int)wbuf_.size());
                if (nwrite > 0) {
                    wbuf_.consume(static_cast<size_t>(nwrite));
                    nbytes += nwrite;
                } else {
                    int ssl_err = SSL_get_error(ssl_, nwrite);
                    if (ssl_err == SSL_ERROR_WANT_READ) {
                        mask |= EVENT_READABLE;
                    } else if (ssl_err == SSL_ERROR_WANT_WRITE) {
                        mask |= EVENT_WRITABLE;
                    } else {
                        err = -1;
                    }
                    break;
                }
            } else {
                break;
            }
        }
        if (nbytes > 0) {
            Invoke(write_callback_);
        }
        if (wbuf_.size() > 0) {
            mask |= EVENT_WRITABLE;
            mask_ &= ~EVENT_WRITABLE;
        }
    }
    if (err == ERROR_OK) {
        if ((mask & EVENT_WRITABLE) == 0) {
            ClearEvent(EVENT_WRITABLE);
        }
        if (mask != EVENT_NONE) {
            if (AddEvent(mask) == -1) {
                err = ERROR_EVENTLOOP_REGISTER;
            }
        }
    }
    if (err != ERROR_OK) {
        SetError(err);
    }
}

void SSLSocket::SSLHandshake() {
    int ssl_ret, ssl_err, err;
    err = 0;
    if ((ssl_ret = SSL_do_handshake(ssl_)) == 1) {
        handshake_status_ = HS_HANDSHAKED;
        Readable();

        Invoke(estab_callback_);
        return;
    }
    ERR_clear_error();
    ssl_err = SSL_get_error(ssl_, ssl_ret);

    if (ssl_err == SSL_ERROR_WANT_WRITE) {
        if (AddEvent(EVENT_WRITABLE) == -1) {
            err = ERROR_EVENTLOOP_REGISTER;
        }
    } else if (ssl_err == SSL_ERROR_WANT_READ) {
        if (AddEvent(EVENT_READABLE) == -1) {
            err = ERROR_EVENTLOOP_REGISTER;
        }
    } else {
        err = -1;

        //const char* file = NULL;
        //int line, flag;
        //const char* data = NULL;
        //line = flag = 0;
        //unsigned long code = ERR_peek_error_line_data(&file, &line, &data, &flag);
        //char buf[1024] = { 0 };
        //const char* e = ERR_error_string(code, buf);
    }
    if (err) {
        SetError(err);
    }
}

}
}

