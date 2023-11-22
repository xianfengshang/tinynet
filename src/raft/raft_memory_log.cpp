// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "raft_node.h"
#include "raft_memory_log.h"
#include "raft.pb.h"

namespace tinynet {
namespace raft {

RaftMemoryLog::RaftMemoryLog() :
    start_index_(1) {
}

uint64_t RaftMemoryLog::end() const {
    return start_index_ + entries_.size();
}

LogEntryPtr RaftMemoryLog::at(uint64_t index) const {
    size_t pos = to_pos(index);
    if (pos == npos) return LogEntryPtr();
    return entries_[pos];
}

size_t RaftMemoryLog::erase(uint64_t first, uint64_t last) {
    first = (std::max)(first, begin());
    last = (std::min)(last, end());
    if (first > last) return 0;
    uint64_t begin_pos = first - begin();
    uint64_t end_pos = last - begin();
    entries_.erase(entries_.begin() + begin_pos, entries_.begin() + end_pos);
    return end_pos - begin_pos;
}

void RaftMemoryLog::append(const std::vector<LogEntryPtr>& entries) {
    for (auto& entry: entries) {
        entries_.push_back(entry);
    }
}

void RaftMemoryLog::reset(uint64_t start_index) {
    erase(start_index_, (std::min)(start_index, end()));
    start_index_ = start_index;
}

size_t RaftMemoryLog::to_pos(uint64_t index) const {
    if (index < begin() || index >= end()) return npos;
    return index - start_index_;
}

}
}
