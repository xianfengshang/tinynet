// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
#ifdef _WIN32
#pragma warning(disable:4819)
#endif
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/indexable.hpp>
#include <boost/geometry/index/equal_to.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include "base/vector3int.h"
#include "base/bounds.h"
#include "tilemap_types.h"

namespace boost {
namespace geometry {

// Adapt the Entity to the boost concept
namespace traits {


template <>
struct tag<tinynet::Vector3Int> {
    typedef point_tag type;
};

template<>
struct coordinate_type<tinynet::Vector3Int> {
    typedef int type;
};

template<>
struct coordinate_system<tinynet::Vector3Int> {
    typedef boost::geometry::cs::cartesian type;
};

template<>
struct dimension<tinynet::Vector3Int >
    : boost::mpl::int_<2>
{};

template<>
struct access<tinynet::Vector3Int, 0 > {
    static inline int get(tinynet::Vector3Int const& p) {
        return p.x;
    }

    static inline void set(tinynet::Vector3Int& p, int  value) {
        p.x = value;
    }
};

template<>
struct access<tinynet::Vector3Int, 1 > {
    static inline int get(tinynet::Vector3Int const& p) {
        return p.y;
    }

    static inline void set(tinynet::Vector3Int& p,int value) {
        p.y = value;
    }
};

//template<>
//struct access<tinynet::Vector3Int, 2 > {
//    static inline float get(
//        tinynet::Vector3Int const& p) {
//        return p.z;
//    }
//
//    static inline void set(tinynet::Vector3Int& p,
//                           float const& value) {
//        p.z = value;
//    }
//};

} // namespace traits
}
}