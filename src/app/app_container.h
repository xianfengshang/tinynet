// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <memory>
#include <atomic>
#include "net/event_loop.h"
#include "tdc/tdc_service.h"
#include "app_types.h"
#include "base/base.h"

#pragma once
namespace tinynet {
namespace cluster {
class ClusterService;
}
namespace http {
class HttpClient;
}
namespace lua {
class LuaScript;
}
namespace tfs {
class TfsService;
}
namespace app {

class AppContainer {
  public:
    AppContainer(int64_t id, const AppMeta &meta);
    ~AppContainer();
  public:
    int Init();
    void Stop();
    int Run();

    int RunForever() noexcept;
    void Exit();
    void Signal(int signum);
  public:
    const AppMeta& meta() { return meta_; }

    EventLoop* event_loop() { return event_loop_.get(); }

    template<class T>
    T* get();
  private:
    int64_t	 id_;
    AppMeta	 meta_;
  private:
    std::unique_ptr<EventLoop>			event_loop_;
    std::unique_ptr<tinynet::tfs::TfsService>	tfs_;
    std::unique_ptr <cluster::ClusterService>	cluster_;
    std::unique_ptr<http::HttpClient>			http_;
    std::unique_ptr<tinynet::lua::LuaScript> 	script_;
};
}
}
