// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "vector3.h"
namespace tinynet {
class Bounds {
  public:
    Bounds(const Vector3& center, const Vector3& size);
    Bounds(const Bounds& other);
    Bounds(Bounds&& other);
    Bounds();
  public:
    Vector3 Min() const {
        return center_ - extents_;
    }

    Vector3 Max() const {
        return center_ + extents_;
    }
    Vector3 Size() const {
        return extents_ * 2;
    }
    void SetSize(const Vector3& size) {
        extents_ = size * 0.5f;
    }
    const Vector3& Center() const {
        return center_;
    }
    void SetCenter(const Vector3& center) {
        center_ = center;
    }
    void SetExtents(const Vector3& extents) {
        extents_ = extents;
    }
    const Vector3& Extents() const {
        return extents_;
    }
    void SetMinMax(const Vector3& min, const Vector3& max) {
        extents_ = (max - min) * 0.5f;
        center_ = min + extents_;
    }
    inline bool operator ==(const Bounds& o) const {
        return center_ == o.center_ && extents_ == o.extents_;
    }
    inline Bounds& operator =(const Bounds& o) {
        center_ = o.center_;
        extents_ = o.extents_;
        return *this;
    }
  private:
    Vector3 center_;
    Vector3 extents_;
};
}