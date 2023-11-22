// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <cmath>
#include <algorithm>

namespace tinynet {
struct Vector3Int {
    static float Distance(const Vector3Int& a, const Vector3Int& b) {
        return (float)std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) *(a.z - b.z));
    }
    static int Dot(const Vector3Int& a, const Vector3Int& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    static Vector3Int Max(const Vector3Int& a, const Vector3Int& b) {
        return { (std::max)(a.x, b.x), (std::max)(a.y, b.y), (std::max)(a.z, b.z) };
    }
    static Vector3Int Min(const Vector3Int& a, const Vector3Int& b) {
        return { (std::min)(a.x, b.x), (std::min)(a.y, b.y), (std::min)(a.z, b.z) };
    }
    int SqrMagnitude() const {
        return x * x + y * y + z * z;
    }
    float Magnitude() const {
        return (float)std::sqrt(x * x + y * y + z * z);
    }

    inline Vector3Int operator +(const Vector3Int& o) const {
        return { x + o.x, y + o.y, z + o.z };
    }
    inline Vector3Int operator-() const {
        return { -x, -y, -z };
    }

    inline Vector3Int operator-(const Vector3Int& o) const {
        return { x - o.x, y - o.y, z - o.z };
    }
    inline Vector3Int operator *( int d) const {
        return { x*d, y*d, z*d };
    }
    inline Vector3Int operator /(int d) const {
        return { x / d, y / d, z / d };
    }
    inline bool operator ==(const Vector3Int& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
    inline bool operator !=(const Vector3Int& o) const {
        return !this->operator==(o);
    }
    static Vector3Int up;
    static Vector3Int down;
    static Vector3Int right;
    static Vector3Int left;
    static Vector3Int forward;
    static Vector3Int back;
    static Vector3Int zero;
    static Vector3Int one;

    int x;
    int y;
    int z;
};

}

inline tinynet::Vector3Int operator *(int d, const tinynet::Vector3Int& a) {
    return a.operator*(d);
}
