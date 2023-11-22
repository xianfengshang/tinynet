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

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
namespace bgm = boost::geometry::model;

namespace tinynet {
namespace geo {
//typedef bgm::point<float, 2, bg::cs::spherical_equatorial<bg::degree>> Point;
typedef bgm::point<double, 2, bg::cs::geographic<bg::degree>> Point;
typedef bgm::multi_point<Point> MultiPoint;
typedef bgm::polygon<Point> Polygon;
typedef bgm::multi_polygon<Polygon> MultiPolygon;
typedef bgm::ring<Point> Ring;
typedef bgm::linestring<Point> LineString;
typedef bgm::multi_linestring<LineString> MultiLineString;

typedef int64_t EntryId;

typedef bgm::box<Point> Box;
typedef std::pair<Point, EntryId> Entry;
typedef Point Coordinate;
}
}



namespace boost {
namespace geometry {

namespace strategy {
namespace centroid {

class geo_average {
  private:

    /*! subclass to keep state */
    class sum {
        friend class geo_average;
        signed_size_type count;
        tinynet::geo::Point centroid;

      public:
        inline sum()
            : count(0) {
            assign_zero(centroid);
        }
    };

  public:
    typedef sum state_type;
    typedef tinynet::geo::Point centroid_point_type;
    typedef tinynet::geo::Point point_type;

    static inline void apply(tinynet::geo::Point const& p1, tinynet::geo::Point const& p2, sum& state) {
        add_point(state.centroid, p2);
        state.count++;
    }

    static inline bool result(sum const& state, tinynet::geo::Point& centroid) {
        centroid = state.centroid;
        if (state.count > 0) {
            divide_value(centroid, (float)state.count);
            return true;
        }
        return false;
    }

};



namespace services {

template<>
struct default_strategy
    <
    geographic_tag,
    pointlike_tag,
    2,
    tinynet::geo::Point,
    tinynet::geo::Polygon
    > {
    typedef centroid::geo_average type;
};

} // namespace services

}
} // namespace strategy::centroid


}
} // namespace boost::geometry