// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
#include <map>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/indexable.hpp>
#include <boost/geometry/index/equal_to.hpp>
#include "base/vector3.h"
#include "base/bounds.h"

namespace tinynet {
namespace aoi {
typedef int64_t EntryId;
typedef std::pair<EntryId, Bounds> Entry;
typedef Vector3 Coordinate;
}
}