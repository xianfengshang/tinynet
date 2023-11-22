// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <deque>
#include <memory>
#include <algorithm>
#include "raft_types.h"
namespace tinynet {

namespace raft {
using LogEntryPtr = std::shared_ptr<LogEntry>;

class RaftMemoryLog {
  public:
    RaftMemoryLog();
  public:
    uint64_t begin() const { return start_index_; }
    uint64_t end() const;
    LogEntryPtr at(uint64_t index) const;
    size_t erase(uint64_t first, uint64_t last);
    void append(const std::vector<LogEntryPtr>& entries);
    size_t size() { return entries_.size(); }
    void reset(uint64_t start_index);
  private:
    size_t to_pos(uint64_t index) const;
  public:
    static const size_t npos = ~0;
  private:
    std::deque<LogEntryPtr> entries_;
    uint64_t start_index_;
};
}
}
