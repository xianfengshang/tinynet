// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "google/protobuf/stubs/common.h"
namespace tinynet {
namespace rpc {
struct ClosureGuard {
    ClosureGuard(::google::protobuf::Closure *cb);
    ~ClosureGuard();
    ::google::protobuf::Closure * callback;
};
}
}
