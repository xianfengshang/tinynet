#pragma once
#include <vector>
#include <string>
#include <tuple>
#include "geo_types.h"

namespace tinynet {
namespace geojson {

struct Geometry {
    std::string type;
};

struct Point {
    tinynet::geo::Point coordinates;
};

struct MultiPoint {
    tinynet::geo::MultiPoint coordinates;
};

struct LineString {
    tinynet::geo::LineString coordinates;
};

struct MultiLineString {
    tinynet::geo::MultiLineString coordinates;
};

struct Polygon {
    std::vector<tinynet::geo::Ring> coordinates;
};

struct MultiPolygon {
    std::vector<std::vector<tinynet::geo::Ring>> coordinates;
};

}
}
