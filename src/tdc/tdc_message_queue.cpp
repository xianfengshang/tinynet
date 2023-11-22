// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "tdc_message_queue.h"
#include <algorithm>
namespace tinynet {
namespace tdc {
TdcMessageQueue::TdcMessageQueue():
    next_(0) {
}

void TdcMessageQueue::Rewind() {
    next_ = 0;
}

TdcMessagePtr TdcMessageQueue::Next() {
    if (next_ >= queue_.size()) {
        return nullptr;
    }
    return queue_[next_++];
}

TdcMessagePtr TdcMessageQueue::Front() {
    return queue_.front();
}

size_t TdcMessageQueue::Size() {
    return queue_.size();
}

size_t TdcMessageQueue::Rsize() {
    return queue_.size() - next_;
}

size_t TdcMessageQueue::Lsize() {
    return next_;
}

void TdcMessageQueue::Run(int error_code) {
    std::deque<TdcMessagePtr> queue;
    queue_.swap(queue);
    while (queue.size() > 0) {
        TdcMessagePtr msg = queue.front();
        queue.pop_front();
        msg->Run(error_code);
    }
    Rewind();
}

void TdcMessageQueue::Push(TdcMessagePtr msg) {
    queue_.emplace_back(std::move(msg));
}

void TdcMessageQueue::Emplace(TdcMessagePtr&& msg) {
    queue_.emplace_back(msg);
}

void TdcMessageQueue::Pop() {
    if (queue_.empty()) {
        return;
    }
    if (next_ > 0) next_--;
    queue_.pop_front();
}
}
}
