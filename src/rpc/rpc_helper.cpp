// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "rpc_helper.h"

namespace tinynet {
namespace rpc {
ClosureGuard::ClosureGuard(::google::protobuf::Closure *cb):
    callback(cb) {
}

ClosureGuard::~ClosureGuard() {
    if (callback) {
        callback->Run();
    }
}
}
}