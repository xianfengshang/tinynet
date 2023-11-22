// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "vector3int.h"

namespace tinynet {
Vector3Int Vector3Int::up = { 0, 1, 0 };
Vector3Int Vector3Int::down = { 0, -1, 0 };
Vector3Int Vector3Int::right = { 1, 0, 0 };
Vector3Int Vector3Int::left = { -1, 0, 0 };
Vector3Int Vector3Int::forward = { 0,0,1 };
Vector3Int Vector3Int::back = { 0, 0, -1 };
Vector3Int Vector3Int::zero = { 0, 0, 0 };
Vector3Int Vector3Int::one = { 1, 1, 1 };

}