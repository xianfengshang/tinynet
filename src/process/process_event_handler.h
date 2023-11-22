// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
namespace tinynet {
namespace process {
class ProcessEventHandler {
  public:
    virtual void HandleExit(int exit_status, int term_signal) = 0;
};
}
}