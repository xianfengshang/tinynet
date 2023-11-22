// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <math.h>
#include "mathf.h"

namespace tinynet {
struct Vector3 {
    static float Distance(const Vector3& a, const Vector3& b) {
        return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) *(a.z - b.z));
    }
    static float Dot(const Vector3& a, const Vector3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    static Vector3 Slerp(const Vector3& a, const Vector3& b, float t) {
        t = Mathf::Clamp(t, 0, 1);
        return { a.x + (b.x - a.x)*t, a.y + (b.y - a.y) *t, a.z + (b.z - a.z) * t };
    }
    static Vector3 Max(const Vector3& a, const Vector3& b) {
        return { fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z) };
    }
    static Vector3 Min(const Vector3& a, const Vector3& b) {
        return { fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z) };
    }
    float SqrMagnitude() const {
        return x * x + y * y + z * z;
    }
    float Magnitude() const {
        return sqrtf(x * x + y * y + z * z);
    }

    void Normalize() {
        auto n = sqrtf(x * x + y * y + z * z);
        if (n > 1e-5f) {
            x /= n;
            y /= n;
            z /= n;
        } else {
            x = 0;
            y = 0;
            z = 0;
        }
    }

    inline Vector3 operator +(const Vector3& o) const {
        return { x + o.x, y + o.y, z + o.z };
    }
    inline Vector3 operator-() const {
        return { -x, -y, -z };
    }

    inline Vector3 operator-(const Vector3& o) const {
        return { x - o.x, y - o.y, z - o.z };
    }
    inline Vector3 operator *( float d) const {
        return { x*d, y*d, z*d };
    }
    inline Vector3 operator /(float d) const {
        return { x / d, y / d, z / d };
    }
    inline bool operator ==(const Vector3& o) const {
        auto v = *this - o;
        return v.SqrMagnitude() < 1e-10;
    }
    inline bool operator !=(const Vector3& o) const {
        return !this->operator==(o);
    }
    static Vector3 up;
    static Vector3 down;
    static Vector3 right;
    static Vector3 left;
    static Vector3 forward;
    static Vector3 back;
    static Vector3 zero;
    static Vector3 one;

    float x;
    float y;
    float z;
};

}

inline tinynet::Vector3 operator *(float d, const tinynet::Vector3& a) {
    return a.operator*(d);
}
