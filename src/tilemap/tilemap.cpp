// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "tilemap.h"

namespace tinynet {
namespace tilemaps {
bool TileMap::Contains(EntryId entryId) {
    auto it = entries_.find(entryId);
    if (it == entries_.end()) {
        return false;
    }
    return rtree_.count(std::make_pair(it->second, it->first)) > 0;
}

void TileMap::Add(EntryId entryId, const Box& bound) {
    auto it = entries_.find(entryId);
    if (it != entries_.end()) {
        auto entry = std::make_pair(it->second, it->first);
        rtree_.remove(entry);

        it->second = bound;

        entry.first = bound;

        rtree_.insert(entry);
        return;
    }
    auto ret = entries_.emplace(entryId, bound);
    if (ret.second)
        rtree_.insert(std::make_pair(bound, entryId));
}

void TileMap::Clear() {
    {
        RTree temp;
        rtree_.swap(temp);
    }
    {
        EntryMap temp;
        entries_.swap(temp);
    }
}

bool TileMap::Remove(EntryId entryId) {
    auto it = entries_.find(entryId);
    if (it == entries_.end())
        return false;
    rtree_.remove(std::make_pair(it->second, it->first));
    entries_.erase(it);
    return true;
}

bool TileMap::TryGetEntry(EntryId entryId, Box* bound) {
    auto it = entries_.find(entryId);
    if (it == entries_.end()) {
        return false;
    }
    if (bound)
        *bound = it->second;

    return true;
}

size_t TileMap::QueryBound(const Box& bound, std::vector<Entry>* output) {
    return rtree_.query(bgi::intersects(bound), std::back_inserter(*output));
}

}
}