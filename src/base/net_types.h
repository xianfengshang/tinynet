// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
namespace tinynet {
namespace net {
enum EventType {
    EVENT_NONE = 0,
    EVENT_READABLE = 1,
    EVENT_WRITABLE = 2,
    EVENT_ERROR = 4
};

constexpr int EVENT_FULL_MASK = EVENT_READABLE | EVENT_WRITABLE | EVENT_ERROR; //full event mask
}
}
