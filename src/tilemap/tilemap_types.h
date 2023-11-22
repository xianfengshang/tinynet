// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
#include <map>
#include <string>
#ifdef _WIN32
#pragma warning(disable:4819)
#endif
#include <boost/geometry.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/index/indexable.hpp>
#include <boost/geometry/index/equal_to.hpp>
#include <boost/geometry/strategies/centroid.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>

#include "base/vector3int.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
namespace bgm = boost::geometry::model;

namespace tinynet {
namespace tilemaps {
typedef Vector3Int Point;
typedef int64_t EntryId;

typedef bgm::box<Point> Box;
typedef std::pair<Box, EntryId> Entry;
typedef Point Coordinate;
}
}