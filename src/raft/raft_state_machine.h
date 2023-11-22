// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "base/io_buffer.h"
#include <string>
#include "raft_types.h"
namespace tinynet {
namespace raft {
class RaftStateMachine {
  public:
    virtual ~RaftStateMachine() {}
  public:
    virtual void StateChanged(StateType role) = 0;
    virtual void SaveSnapshot(IOBuffer* buffer) = 0;
    virtual void LoadSnapshot(const char* data, size_t len) = 0;
    virtual void ApplyEntry(uint64_t logIndex, const std::string& data) = 0;
};
}

}
