// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "geo_types.h"
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>


namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
namespace bgm = boost::geometry::model;

namespace tinynet {
namespace geo {

struct GeoSearchResult {
    EntryId entryId;
    double distance;
};

class GeoService {
  public:
    bool Contains(EntryId entryId);
    void Add(EntryId entryId, const Coordinate& pos);
    void Update(EntryId entryId, const Coordinate& pos);
    bool Remove(EntryId entryId);
    bool TryGetEntry(EntryId entryId, Coordinate* pos);
    void Clear();

    size_t Size() const { return entries_.size(); }
    size_t QueryRadius(const Coordinate& pos, double radius, std::vector<GeoSearchResult>* output);
    size_t QueryNearest(EntryId entryId, int n, std::vector<GeoSearchResult>* output);
  private:
    void Upsert(EntryId entryId, const Coordinate& pos);
  public:
    typedef bgi::rtree<Entry, bgi::linear<16, 4>> RTree;
    typedef std::map<EntryId, Coordinate> EntryMap;
  private:
    EntryMap entries_;
    RTree rtree_;

};
}
}