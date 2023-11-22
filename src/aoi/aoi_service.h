// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "aoi_types.h"
#include "aoi_traits.h"
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include "base/bounds.h"


namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
namespace bgm = boost::geometry::model;

namespace tinynet {
namespace aoi {
class AoiService {
  public:
    bool Contains(EntryId entryId);
    bool TryGetEntry(EntryId entryId, Coordinate* pos, Vector3* size);
    void Add(EntryId entryId, const Coordinate& pos, const Vector3& size);
    void Clear();
    bool Remove(EntryId entryId);
    size_t QueryBound(const tinynet::Bounds& bound, std::vector<Entry>* output);
    size_t Size() const { return entries_.size();}
  public:
    typedef bgi::rtree<Entry, bgi::linear<16, 4>> RTree;
    typedef std::map<EntryId, Bounds> EntryMap;
  private:
    EntryMap entries_;
    RTree rtree_;

};
}
}