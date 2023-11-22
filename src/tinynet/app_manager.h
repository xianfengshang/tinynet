// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <vector>
#include <thread>
#include "base/module.h"
#include "app/app_container.h"
#include "net/event_loop.h"

class AppManager :
    public tinynet::Module<AppManager> {
  public:
    int Start();
    int Stop();
    int Signal(int signum);
    int Run();
  private:
    std::vector<std::unique_ptr<tinynet::app::AppContainer> > apps_;
    std::vector<std::unique_ptr<std::thread> > threads_;
    std::unique_ptr<tinynet::EventLoop> event_loop_;

};
