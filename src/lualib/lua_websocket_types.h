// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
namespace tinynet {
namespace lua {
struct WebSocketEvent {
    int64_t guid{ 0 };
    std::string addr;
    std::string type;
    std::string data;
    const std::string* data_ref{ 0 };
};
}
}
