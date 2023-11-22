// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "geo_service.h"

namespace tinynet {
namespace geo {
bool GeoService::Contains(EntryId entryId) {
    auto it = entries_.find(entryId);
    if (it == entries_.end()) {
        return false;
    }
    return rtree_.count(std::make_pair(it->second, it->first)) > 0;
}

void GeoService::Add(EntryId entryId, const Coordinate& pos) {
    Upsert(entryId, pos);
}

void GeoService::Update(EntryId entryId, const Coordinate& pos) {
    Upsert(entryId, pos);
}

void GeoService::Clear() {
    {
        RTree temp;
        rtree_.swap(temp);
    }
    {
        EntryMap temp;
        entries_.swap(temp);
    }
}

bool GeoService::Remove(EntryId entryId) {
    auto it = entries_.find(entryId);
    if (it == entries_.end())
        return false;
    rtree_.remove(std::make_pair(it->second, it->first));
    entries_.erase(it);
    return true;
}

bool GeoService::TryGetEntry(EntryId entryId, Coordinate* pos) {
    auto it = entries_.find(entryId);
    if (it == entries_.end()) {
        return false;
    }
    if (pos != nullptr) {
        *pos = it->second;
    }
    return true;
}

void GeoService::Upsert(EntryId entryId, const Coordinate& pos) {
    auto it = entries_.find(entryId);
    if (it != entries_.end()) {
        auto entry = std::make_pair(it->second, it->first);
        rtree_.remove(entry);

        it->second = pos;

        entry.first = pos;

        rtree_.insert(entry);
        return;
    }
    auto ret = entries_.emplace(entryId, pos);
    if (ret.second)
        rtree_.insert(std::make_pair(pos, entryId));
}

size_t GeoService::QueryRadius(const Coordinate& pos, double radius, std::vector<GeoSearchResult>* output) {
    Point posMin = { pos.get<0>() - radius, pos.get<1>() - radius };
    Point posMax = { pos.get<0>() + radius, pos.get<1>() + radius };
    Box bound(posMin, posMax);
    std::vector<tinynet::geo::Entry> raw_output;
    double dis;
    size_t nret = rtree_.query(bgi::within(bound), std::back_inserter(raw_output));
    if (nret > 0 && output) {
        for (auto& value : raw_output) {
            if ((dis = bg::distance(value.first, pos)) <= radius) {
                GeoSearchResult res{ value.second, dis };
                output->emplace_back(std::move(res));
            }
        }
    }
    return nret;
}

size_t GeoService::QueryNearest(EntryId entryId, int n, std::vector<GeoSearchResult>* output) {
    auto it = entries_.find(entryId);
    if (it == entries_.end())
        return 0;
    auto pos = it->second;
    std::vector<tinynet::geo::Entry> raw_output;
    size_t nret = rtree_.query(bgi::nearest(pos, static_cast<unsigned int>(n)), std::back_inserter(raw_output));
    if (nret > 0 && output) {
        for (auto& value : raw_output) {
            if (value.second != entryId) {
                GeoSearchResult res{ value.second, bg::distance(value.first, pos) };
                output->emplace_back(std::move(res));
            }
        }
    }
    return nret;
}

}
}