// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "tilemap_types.h"
#include "tilemap_traits.h"
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>


namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
namespace bgm = boost::geometry::model;

namespace tinynet {
namespace tilemaps {

class TileMap {
  public:
    bool Contains(EntryId entryId);
    void Add(EntryId entryId, const Box& bound);
    bool Remove(EntryId entryId);
    bool TryGetEntry(EntryId entryId, Box* bound);
    void Clear();

    size_t Size() const { return entries_.size(); }
    size_t QueryBound(const Box& bound, std::vector<Entry>* output);
  public:
    typedef bgi::rtree<Entry, bgi::linear<16, 4>> RTree;
    typedef std::map<EntryId, Box> EntryMap;
  private:
    EntryMap entries_;
    RTree rtree_;

};
}
}