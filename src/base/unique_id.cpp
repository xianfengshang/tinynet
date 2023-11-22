// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "unique_id.h"
#include "id_allocator.h"
#include <memory>

namespace tinynet {
UniqueId NewUniqueId() {
    return IdAllocator::Instance()->NextId();
}
}
