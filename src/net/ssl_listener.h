// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma  once
#include <memory>
#include "event_loop.h"
#include "listener.h"
namespace tinynet {

namespace net {
//Listener for SSL server
class SSListener:
    public Listener {
  public:
    SSListener(EventLoop *loop, SSLContext* ctx);
    ~SSListener();
  private:
    virtual void Readable() override;
  private:
    SSLContext* ssl_ctx_;
};

typedef std::shared_ptr<SSListener> SSListenerPtr;
}
}
