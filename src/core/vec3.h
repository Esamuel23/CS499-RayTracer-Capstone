//=============================================================================
// File:     vec3.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Three-component vector used for points, directions, and colors.
//
// Design notes:
//   The original CS 330 artifact used glm::vec3 and handed every value to
//   OpenGL. The ray tracer has no graphics API, so it owns its math.
//   Vec3 is a small value type. It is passed by value or const reference,
//   never allocated on the heap, and has no hidden state.
//=============================================================================
#pragma once

#include <cmath>

namespace rt {

class Vec3 {
public:
    constexpr Vec3() = default;
    constexpr Vec3(double x, double y, double z) : x_(x), y_(y), z_(z) {}

    constexpr double x() const { return x_; }
    constexpr double y() const { return y_; }
    constexpr double z() const { return z_; }

    // Axis access for loops over x, y, z (used by the slab test).
    // Any index other than 0 or 1 returns z, so the call can never read
    // outside the object.
    constexpr double operator[](int axis) const {
        return axis == 0 ? x_ : (axis == 1 ? y_ : z_);
    }

    constexpr Vec3 operator-() const { return Vec3(-x_, -y_, -z_); }

    Vec3& operator+=(const Vec3& other) {
        x_ += other.x_;
        y_ += other.y_;
        z_ += other.z_;
        return *this;
    }

    Vec3& operator*=(double scalar) {
        x_ *= scalar;
        y_ *= scalar;
        z_ *= scalar;
        return *this;
    }

    Vec3& operator/=(double scalar) { return *this *= (1.0 / scalar); }

    constexpr double length_squared() const { return x_ * x_ + y_ * y_ + z_ * z_; }
    double length() const { return std::sqrt(length_squared()); }

    // True when every component is close to zero. Used to reject
    // degenerate scatter directions instead of normalizing a zero vector.
    bool near_zero() const {
        constexpr double kTolerance = 1e-8;
        return std::fabs(x_) < kTolerance && std::fabs(y_) < kTolerance &&
               std::fabs(z_) < kTolerance;
    }

private:
    double x_ = 0.0;
    double y_ = 0.0;
    double z_ = 0.0;
};

// Aliases make intent clear at call sites. They are the same type.
using Point3 = Vec3;
using Color = Vec3;

constexpr Vec3 operator+(const Vec3& a, const Vec3& b) {
    return Vec3(a.x() + b.x(), a.y() + b.y(), a.z() + b.z());
}

constexpr Vec3 operator-(const Vec3& a, const Vec3& b) {
    return Vec3(a.x() - b.x(), a.y() - b.y(), a.z() - b.z());
}

// Component-wise product. Used to tint light by a surface color.
constexpr Vec3 operator*(const Vec3& a, const Vec3& b) {
    return Vec3(a.x() * b.x(), a.y() * b.y(), a.z() * b.z());
}

constexpr Vec3 operator*(double scalar, const Vec3& v) {
    return Vec3(scalar * v.x(), scalar * v.y(), scalar * v.z());
}

constexpr Vec3 operator*(const Vec3& v, double scalar) { return scalar * v; }

constexpr Vec3 operator/(const Vec3& v, double scalar) { return (1.0 / scalar) * v; }

constexpr double dot(const Vec3& a, const Vec3& b) {
    return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}

constexpr Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(a.y() * b.z() - a.z() * b.y(),
                a.z() * b.x() - a.x() * b.z(),
                a.x() * b.y() - a.y() * b.x());
}

// Caller must not pass a zero vector. Every call site either builds
// the vector from nonzero inputs or checks near_zero() first.
inline Vec3 unit_vector(const Vec3& v) { return v / v.length(); }

// Mirror reflection of v about surface normal n (n must be unit length).
constexpr Vec3 reflect(const Vec3& v, const Vec3& n) { return v - 2.0 * dot(v, n) * n; }

// Snell's law refraction. uv and n must be unit length.
// eta_ratio is (index of incident medium) / (index of transmitted medium).
inline Vec3 refract(const Vec3& uv, const Vec3& n, double eta_ratio) {
    const double cos_theta = std::fmin(dot(-uv, n), 1.0);
    const Vec3 perpendicular = eta_ratio * (uv + cos_theta * n);
    const Vec3 parallel = -std::sqrt(std::fabs(1.0 - perpendicular.length_squared())) * n;
    return perpendicular + parallel;
}

}  // namespace rt
