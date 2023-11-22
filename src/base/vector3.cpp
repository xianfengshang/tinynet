// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "vector3.h"

namespace tinynet {
Vector3 Vector3::up = { 0.0f, 1.0f, 0.0f };
Vector3 Vector3::down = { 0.0f, -1.0f, 0.0f };
Vector3 Vector3::right = { 1.0f, 0.0f, 0.0f };
Vector3 Vector3::left = { -1.0f, 0.0f, 0.0f };
Vector3 Vector3::forward = { 0.0f,0.0f,1.0f };
Vector3 Vector3::back = { 0.0f, 0.0f, -1.0f };
Vector3 Vector3::zero = { 0.0f, 0.0f, 0.0f };
Vector3 Vector3::one = { 1.0f, 1.0f, 1.0f };

}