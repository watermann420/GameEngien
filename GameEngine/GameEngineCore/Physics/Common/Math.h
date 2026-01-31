#pragma once
#include <cmath>

struct Vec2
{
    double x{ 0.0 };
    double y{ 0.0 };

    Vec2() = default;
    Vec2(double x_, double y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return { x + o.x, y + o.y }; }
    Vec2 operator-(const Vec2& o) const { return { x - o.x, y - o.y }; }
    Vec2 operator*(double s) const { return { x * s, y * s }; }
    Vec2 operator/(double s) const { return { x / s, y / s }; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(double s) { x *= s; y *= s; return *this; }

    double LengthSq() const { return x * x + y * y; }
    double Length() const { return std::sqrt(LengthSq()); }
    Vec2 Normalized() const { double len = Length(); return len > 0 ? (*this) / len : Vec2{}; }
};

struct Vec3
{
    double x{ 0.0 };
    double y{ 0.0 };
    double z{ 0.0 };

    Vec3() = default;
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator*(double s) const { return { x * s, y * s, z * s }; }
    Vec3 operator/(double s) const { return { x / s, y / s, z / s }; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(double s) { x *= s; y *= s; z *= s; return *this; }

    double LengthSq() const { return x * x + y * y + z * z; }
    double Length() const { return std::sqrt(LengthSq()); }
    Vec3 Normalized() const { double len = Length(); return len > 0 ? (*this) / len : Vec3{}; }
};

