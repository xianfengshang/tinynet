// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifdef _WIN32
#include "poller_iocp.h"
#include "poller.h"
#include "base/net_types.h"
#include "base/winsock_manager.h"
#include "util/net_utils.h"
#include "event_loop.h"

namespace tinynet {
namespace net {
PollerIocp::PollerIocp(EventLoop* loop)
    :PollerImpl(loop),
     handle_(NULL) {
}

bool PollerIocp::Init() {
    handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
    if (handle_ == NULL) {
        return false;
    }
    return true;
}

void PollerIocp::Stop() {
    if (handle_ != NULL) {
        CloseHandle(handle_);
        handle_ = NULL;
    }
}

const char* PollerIocp::name() {
    return "iocp";
}

int PollerIocp::Poll(poll_event* events, int maxevents, int timeout_ms) {
    int nevents = 0;
    ULONG ncompleted = 0;
    maxevents = (std::min)((int)MAX_OVERLAMPPED_ENTRIES, maxevents);
    BOOL res = WinsockManager::WinAPI_GetQueuedCompletionStatusEx(handle_, &entries_[0], maxevents, &ncompleted, timeout_ms, FALSE);
    if (!res || ncompleted == 0) {
        return nevents;
    }
    for (size_t i = 0; i < ncompleted && nevents < maxevents; ++i) {
        LPOVERLAPPED_ENTRY pEntry = &entries_[i];
        if (!pEntry->lpOverlapped) continue;

        int fd = (int)pEntry->lpCompletionKey;
        sock_t* sock = g_WinsockManager->GetSocket(fd);
        if (!sock) continue;

        NTSTATUS status = (NTSTATUS)(pEntry->Internal);
        int mask = 0;
        if (status >= 0) {
            req_t* req = CONTAINING_RECORD(pEntry->lpOverlapped, req_t, overlapped);
            switch (req->type) {
            case NONE_REQ: {
                continue;
            }
            case READ_REQ: {
                if (sock->flags & FD_FLAG_READ_QUEUED) {
                    sock->flags &= ~FD_FLAG_READ_QUEUED;
                    mask |= EVENT_READABLE;
                }
                break;
            }
            case READ_REQ2: {
                if (sock->flags & FD_FLAG_READ_QUEUED2) {
                    sock->flags &= ~FD_FLAG_READ_QUEUED2;
                    mask |= NetUtils::WaitEvent(sock->fd, EVENT_READABLE | EVENT_ERROR, 5);
                    if (mask == EVENT_NONE) {
                        if (g_WinsockManager->PostEvent(handle_, fd, READ_REQ2) == -1) {
                            mask |= EVENT_READABLE;
                            mask |= EVENT_ERROR;
                        } else {
                            continue;
                        }
                    }
                }
                break;
            }
            case WRITE_REQ: {
                if (sock->flags & FD_FLAG_WRITE_QUEUED) {
                    sock->flags &= ~FD_FLAG_WRITE_QUEUED;
                    mask |= EVENT_WRITABLE;
                }
                break;
            }
            case WRITE_REQ2: {
                if (sock->flags & FD_FLAG_WRITE_QUEUED2) {
                    sock->flags &= ~FD_FLAG_WRITE_QUEUED2;
                    mask |= NetUtils::WaitEvent(sock->fd, EVENT_WRITABLE | EVENT_ERROR, 5);
                    if (mask == EVENT_NONE) {
                        if (g_WinsockManager->PostEvent(handle_, fd, WRITE_REQ2) == -1) {
                            mask |= EVENT_READABLE;
                            mask |= EVENT_ERROR;
                        } else {
                            continue;
                        }
                    }
                }
                break;
            }
            case CONNECT_REQ: {
                if (sock->flags & FD_FLAG_CONNECT_PENDING) {
                    sock->flags &= ~FD_FLAG_CONNECT_PENDING;
                    mask |= EVENT_WRITABLE;
                    g_WinsockManager->UpdateConnectContext(fd);
                }
                break;
            }
            case ACCEPT_REQ: {
                accept_req_t* accept_req = reinterpret_cast<accept_req_t*>(req);
                if (mask == EVENT_NONE) {
                    g_WinsockManager->QueuedIncomingConnection(fd, accept_req);
                }
                mask |= EVENT_READABLE;
                break;
            }
            case CLOSE_REQ: {
                if (sock->flags & FD_FLAG_CLOSE_PENDING) {
                    sock->flags &= ~FD_FLAG_CLOSE_PENDING;
                    sock->flags |= FD_FLAG_CLOSE_DONE;
                    mask |= EVENT_ERROR;
                }
                break;
            }
            default:
                break;
            }
        } else {
            mask |= EVENT_ERROR;
        }
        if (mask != EVENT_NONE) {
            events[nevents].mask = mask;
            events[nevents].fd = fd;
            nevents++;
        }
    }
    return nevents;
}

int PollerIocp::Add(int fd, int mask) {
    sock_t* sock = g_WinsockManager->GetSocket(fd);
    if (!sock) {
        errno = EBADF;
        return -1;
    }
    bool newly_attached = false;
    if ((sock->flags & FD_FLAG_ATTACHED) == 0) {
        if (NetUtils::SetNonBlocking(fd) != 0) {
            return -1;
        }
        SOCKET hfile = (SOCKET)fd;
        int err = 0;
        if (CreateIoCompletionPort((HANDLE)hfile, handle_, fd, 0) == NULL &&
                (err = GetLastError()) != ERROR_INVALID_PARAMETER) {
            errno = err;
            return -1;
        }
        sock->flags |= FD_FLAG_ATTACHED;
        newly_attached = err == 0;
    }
    if (mask & EVENT_READABLE) {
        if (sock->flags & FD_FLAG_LISTENING) {
            if (WinsockManager::PostAccept(handle_, fd) != 0) {
                return -1;
            }
        } else {
            if ((sock->flags & FD_FLAG_READ_QUEUED) == 0) {
                if (WinsockManager::PostRecv(handle_, fd, NULL, 0) != 0) {
                    return -1;
                }
            }
        }
    }
    if (mask & EVENT_WRITABLE) {
        if (newly_attached) {
            if ((sock->flags & FD_FLAG_WRITE_QUEUED2) == 0) {
                if (WinsockManager::PostEvent(handle_, fd, WRITE_REQ2) != 0) {
                    return -1;
                }
            }
        } else {
            if ((sock->flags & FD_FLAG_WRITE_QUEUED) == 0) {
                if (WinsockManager::PostSend(handle_, fd, NULL, 0) != 0) {
                    return -1;
                }
            }
        }
    }
    return 0;
}

int PollerIocp::Del(int fd, int mask) {
    int events = event_loop_->get_poller()->GetEvents(fd);
    int newmask = events & (~mask);
    if (newmask == EVENT_NONE) {
        sock_t* sock = g_WinsockManager->GetSocket(fd);
        if (sock) {
            sock->flags = 0;
        }
    }
    return 0;
}
}
}
#endif
