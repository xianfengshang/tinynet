// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "bounds.h"
namespace tinynet {
Bounds::Bounds(const Vector3& center, const Vector3& size) {
    center_ = center;
    extents_ = size * 0.5f;
}

Bounds::Bounds(const Bounds& other) = default;
Bounds::Bounds(Bounds&& other) = default;
Bounds::Bounds() = default;

}