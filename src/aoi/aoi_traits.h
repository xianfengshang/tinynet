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
#include "base/vector3.h"
#include "base/bounds.h"
#include "aoi_types.h"

namespace boost {
namespace geometry {

// Adapt the Entity to the boost concept
namespace traits {


template <>
struct tag<tinynet::Vector3> {
    typedef point_tag type;
};

template<>
struct coordinate_type<tinynet::Vector3> {
    typedef float type;
};

template<>
struct coordinate_system<tinynet::Vector3> {
    typedef boost::geometry::cs::cartesian type;
};

template<>
struct dimension<tinynet::Vector3 >
    : boost::mpl::int_<2>
{};

template<>
struct access<tinynet::Vector3, 0 > {
    static inline float get(
        tinynet::Vector3 const& p) {
        return p.x;
    }

    static inline void set(tinynet::Vector3& p, float  value) {
        p.x = value;
    }
};

template<>
struct access<tinynet::Vector3, 1 > {
    static inline float get(tinynet::Vector3 const& p) {
        return p.y;
    }

    static inline void set(tinynet::Vector3& p, float  value) {
        p.y = value;
    }
};

//template<>
//struct access<tinynet::Vector3, 2 > {
//    static inline float get(
//        tinynet::Vector3 const& p) {
//        return p.z;
//    }
//
//    static inline void set(tinynet::Vector3& p,
//                           float const& value) {
//        p.z = value;
//    }
//};

} // namespace traits
}
}

namespace boost {
namespace geometry {
namespace index {
namespace detail {
template <>
struct indexable<tinynet::aoi::Entry, false> {
    typedef tinynet::aoi::Entry value_type;


    /*! \brief The type of result returned by function object. */
    typedef tinynet::Bounds const& result_type;

    /*!
    \brief Return indexable extracted from the value.

    \param v The value.
    \return The indexable.
    */
    inline result_type operator()(value_type const& v) const {
        return v.second;
    }

    /*!
    \brief Prevent reference to temporary for types convertible to Value.
    */
    template <typename V>
    inline result_type operator()(V const& v) const {
        return indexable_prevent_any_type<tinynet::Bounds>(v);
    }
};


template <>
struct equal_to<tinynet::aoi::Entry, false> {
    /*! \brief The type of result returned by function object. */
    typedef bool result_type;
    template <typename Strategy>
    inline bool operator()(tinynet::aoi::Entry const& l, tinynet::aoi::Entry const& r,
                           Strategy const& strategy) const {
        return l.first == r.first && l.second == r.second;
    }
};
}
}
}
}
namespace boost {
namespace geometry {
namespace traits {

template <>
struct tag<tinynet::Bounds > {
    typedef box_tag type;
};

template <>
struct point_type<tinynet::Bounds > {
    typedef tinynet::Vector3 type;
};

template <>
struct indexed_access<tinynet::Bounds, min_corner, 0> {
    typedef typename geometry::coordinate_type<tinynet::Vector3>::type coordinate_type;

    static inline coordinate_type get(tinynet::Bounds const& b) {
        return b.Min().x;
    }

    static inline void set(tinynet::Bounds& b, coordinate_type const& value) {
        auto min = b.Min();
        min.x = value;
        b.SetMinMax(min, b.Max());
    }
};

template <>
struct indexed_access<tinynet::Bounds, min_corner, 1> {
    typedef typename geometry::coordinate_type<tinynet::Vector3>::type coordinate_type;

    static inline coordinate_type get(tinynet::Bounds const& b) {
        return b.Min().y;
    }

    static inline void set(tinynet::Bounds& b, coordinate_type const& value) {
        auto min = b.Min();
        min.y = value;
        b.SetMinMax(min, b.Max());
    }
};

//template <>
//struct indexed_access<tinynet::Bounds, min_corner, 2> {
//    typedef typename geometry::coordinate_type<tinynet::Vector3>::type coordinate_type;
//
//    static inline coordinate_type get(tinynet::Bounds const& b) {
//        return b.Min().z;
//    }
//
//    static inline void set(tinynet::Bounds& b, coordinate_type const& value) {
//        auto min = b.Min();
//        min.z = value;
//        b.SetMinMax(min, b.Max());
//    }
//};



template <>
struct indexed_access<tinynet::Bounds, max_corner, 0> {
    typedef typename geometry::coordinate_type<tinynet::Vector3>::type coordinate_type;

    static inline coordinate_type get(tinynet::Bounds const& b) {
        return b.Max().x;
    }

    static inline void set(tinynet::Bounds& b, coordinate_type const& value) {
        auto max = b.Max();
        max.x = value;
        b.SetMinMax(b.Min(), max);
    }
};

template <>
struct indexed_access<tinynet::Bounds, max_corner, 1> {
    typedef typename geometry::coordinate_type<tinynet::Vector3>::type coordinate_type;

    static inline coordinate_type get(tinynet::Bounds const& b) {
        return b.Max().y;
    }

    static inline void set(tinynet::Bounds& b, coordinate_type const& value) {
        auto max = b.Max();
        max.y = value;
        b.SetMinMax(b.Min(), max);
    }
};

//template <>
//struct indexed_access<tinynet::Bounds, max_corner, 2> {
//    typedef typename geometry::coordinate_type<tinynet::Vector3>::type coordinate_type;
//
//    static inline coordinate_type get(tinynet::Bounds const& b) {
//        return b.Max().z;
//    }
//
//    static inline void set(tinynet::Bounds& b, coordinate_type const& value) {
//        auto max = b.Max();
//        max.z = value;
//        b.SetMinMax(b.Min(), max);
//    }
//};

} // namespace traits
}
}