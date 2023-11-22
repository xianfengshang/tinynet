// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "tdc_message.h"
#include <deque>
#include <list>
namespace tinynet {
namespace tdc {
class TdcMessageQueue {
  public:
    TdcMessageQueue();
    TdcMessageQueue(const TdcMessageQueue&) = delete;
    TdcMessageQueue(TdcMessageQueue&&) = delete;
    TdcMessageQueue& operator=(const TdcMessageQueue&) = delete;
  public:
    void Rewind();
    TdcMessagePtr Next();
    TdcMessagePtr Front();
    void Run(int error_code);
    size_t Size();
    size_t Rsize();
    size_t Lsize();
    void Push(TdcMessagePtr msg);
    void Emplace(TdcMessagePtr&& msg);
    void Pop();
  private:
    std::deque<TdcMessagePtr> queue_;
    size_t next_;
};
}
}
