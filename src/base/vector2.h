// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <math.h>
#include "mathf.h"

namespace tinynet {
struct Vector2 {
    static float Distance(const Vector2& a, const Vector2& b) {
        return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
    }
    static float Dot(const Vector2& a, const Vector2& b) {
        return a.x * b.x + a.y * b.y;
    }
    static Vector2 Slerp(const Vector2& a, const Vector2& b, float t) {
        t = Mathf::Clamp(t, 0, 1);
        return { a.x + (b.x - a.x)*t, a.y + (b.y - a.y) *t};
    }
    static Vector2 Max(const Vector2& a, const Vector2& b) {
        return { fmaxf(a.x, b.x), fmaxf(a.y, b.y)};
    }
    static Vector2 Min(const Vector2& a, const Vector2& b) {
        return { fminf(a.x, b.x), fminf(a.y, b.y)};
    }
    float SqrMagnitude() const {
        return x * x + y * y;
    }
    float Magnitude() const {
        return sqrtf(x * x + y * y);
    }

    void Normalize() {
        auto n = sqrtf(x * x + y * y);
        if (n > 1e-5f) {
            x /= n;
            y /= n;
        } else {
            x = 0;
            y = 0;
        }
    }
    inline Vector2 operator +(const Vector2& o) const {
        return {x + o.x, y + o.y};
    }
    inline Vector2 operator-() const {
        return {-x, -y};
    }

    inline Vector2 operator-(const Vector2& o) const {
        return {x - o.x, y - o.y};
    }
    inline Vector2 operator *( float d) const {
        return {x * d, y * d};
    }
    inline Vector2 operator /(float d) const {
        return {x / d, y / d};
    }
    inline bool operator ==(const Vector2& o) const {
        auto v = *this - o;
        return v.SqrMagnitude() < 9.999999e-11;
    }
    inline bool operator !=(const Vector2& o) const {
        return !this->operator==(o);
    }
    static Vector2 up;
    static Vector2 right;
    static Vector2 zero;
    static Vector2 one;

    float x;
    float y;
};

}

inline tinynet::Vector2 operator *(float d, const tinynet::Vector2& a) {
    return a.operator*(d);
}
