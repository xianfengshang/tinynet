// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "aoi_service.h"
#include "base/bounds.h"

namespace tinynet {
namespace aoi {
bool AoiService::Contains(EntryId entryId) {
    auto it = entries_.find(entryId);
    if (it == entries_.end()) {
        return false;
    }
    return rtree_.count(*it) > 0;
}

bool AoiService::TryGetEntry(EntryId entryId, Coordinate* pos, Vector3* size) {
    auto it = entries_.find(entryId);
    if (it == entries_.end()) {
        return false;
    }
    if (pos)
        *pos = it->second.Center();
    if (size)
        *size = it->second.Size();
    return true;
}

void AoiService::Add(EntryId entryId, const Coordinate& pos, const Vector3& size) {
    auto it = entries_.find(entryId);
    if (it != entries_.end()) {
        rtree_.remove(*it);

        it->second.SetCenter(pos);
        it->second.SetSize(size);

        rtree_.insert(*it);
        return;
    }
    auto ret = entries_.emplace(entryId, Bounds(pos, size));
    if (ret.second)
        rtree_.insert(*ret.first);
}


void AoiService::Clear() {
    {
        RTree temp;
        rtree_.swap(temp);
    }
    {
        EntryMap temp;
        entries_.swap(temp);
    }
}

bool AoiService::Remove(EntryId entryId) {
    auto it = entries_.find(entryId);
    if (it == entries_.end())
        return false;
    rtree_.remove(*it);
    entries_.erase(it);
    return true;
}

size_t AoiService::QueryBound(const tinynet::Bounds& bound, std::vector<Entry>* output) {
    return rtree_.query(bgi::intersects(bound), std::back_inserter(*output));
}


}
}