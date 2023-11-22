// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <cstdint>
#include <string>

namespace tinynet {
namespace lua {
struct TcpSocketEvent {
    int64_t guid{ 0 };
    std::string type;
    std::string data;
};
}
}
