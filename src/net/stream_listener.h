// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma  once
#include <memory>
#include "event_loop.h"
#include "listener.h"
namespace tinynet {

namespace net {
//Listener for stream server
class StreamListener:
    public Listener {
  public:
    StreamListener(EventLoop *loop);
    ~StreamListener();
  private:
    void Readable() override;
};
typedef std::shared_ptr<StreamListener> StreamListenerPtr;

}
}