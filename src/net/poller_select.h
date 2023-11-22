// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <unordered_set>
#include "poller_impl.h"
#ifdef _WIN32
#include <WinSock2.h>
#define FD_SETSIZE_EXTEND 4096
struct fd_set_t: public fd_set {
    SOCKET  fd_array_extend[FD_SETSIZE_EXTEND - FD_SETSIZE];   /* an array of SOCKETs */
};
#define FD_ZERO2 FD_ZERO

#define FD_SET2(fd, set) do { \
    u_int __i; \
    for (__i = 0; __i < ((fd_set FAR *)(set))->fd_count; __i++) { \
        if (((fd_set FAR *)(set))->fd_array[__i] == (fd)) { \
            break; \
        } \
    } \
    if (__i == ((fd_set FAR *)(set))->fd_count) { \
        if (((fd_set FAR *)(set))->fd_count < FD_SETSIZE_EXTEND) { \
            ((fd_set FAR *)(set))->fd_array[__i] = (fd); \
            ((fd_set FAR *)(set))->fd_count++; \
        } \
    } \
} while(0, 0)

#else
#include <sys/select.h>
#define FD_SETSIZE_EXTEND 4096
struct fd_set_t: public fd_set {
    __fd_mask fds_bits_extend[FD_SETSIZE_EXTEND / __NFDBITS - FD_SETSIZE / __NFDBITS];
};
#define FD_ZERO2(s) \
  do {                                                                        \
    unsigned int __i;                                                         \
    __fd_mask *p;                                                    	      \
    fd_set_t *__arr = (s);                                                    \
    p = __FDS_BITS (__arr);                                                   \
    for (__i = 0; __i < sizeof (fd_set_t) / sizeof (__fd_mask); ++__i){       \
      p[__i] = 0;                                            				  \
    }                                                                         \
  } while (0)

#define FD_SET2 FD_SET

#endif

namespace tinynet {
namespace net {
class PollerSelect : public PollerImpl {
  public:
    PollerSelect(EventLoop* loop);
  public:
    virtual bool Init();
    virtual void Stop();
    virtual const char* name() override;
    virtual int Poll(poll_event* events, int maxevents, int timeout_ms) override;
    virtual int Add(int fd, int mask) override;
    virtual int Del(int fd, int mask)  override;
  private:
    struct select_fd {
        int64_t rtime{ 0 };
        int64_t wtime{ 0 };
    };
  private:
    std::unordered_set<int> rfd_set_;
    std::unordered_set<int> wfd_set_;
    fd_set_t rfd_;
    fd_set_t wfd_;
};
}
}
