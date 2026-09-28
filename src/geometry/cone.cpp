//=============================================================================
// File:     cone.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Ray intersection with an upright cone and its base disk.
//=============================================================================
#include "geometry/cone.h"

#include <cmath>
#include <stdexcept>
#include <utility>

#include "core/constants.h"

namespace rt {

Cone::Cone(const Point3& center, double base_radius, double height,
           std::shared_ptr<const Material> material)
    : center_(center),
      base_radius_(base_radius),
      half_height_(0.5 * height),
      slope_squared_(0.0),
      material_(std::move(material)) {
    if (!(base_radius_ > 0.0 && height > 0.0)) {
        throw std::invalid_argument("Cone radius and height must be positive");
    }
    if (!material_) {
        throw std::invalid_argument("Cone requires a material");
    }
    const double slope = base_radius_ / height;
    slope_squared_ = slope * slope;
}

bool Cone::hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const {
    const double y_base = center_.y() - half_height_;
    const double y_tip = center_.y() + half_height_;

    // Distances are measured from the axis (x, z) and down from the tip (y).
    // With depth = y_tip - y, the side is: x^2 + z^2 = k^2 * depth^2.
    const double ox = ray.origin().x() - center_.x();
    const double oz = ray.origin().z() - center_.z();
    const double o_depth = y_tip - ray.origin().y();
    const double dx = ray.direction().x();
    const double dy = ray.direction().y();
    const double dz = ray.direction().z();
    const double k2 = slope_squared_;

    double closest = t_max;
    bool found = false;
    Vec3 outward_normal;

    // Accept a candidate side hit only if it lies between base and tip.
    auto try_side = [&](double t) {
        if (t <= t_min || t >= closest) {
            return;
        }
        const Point3 point = ray.at(t);
        if (point.y() < y_base || point.y() > y_tip) {
            return;
        }
        closest = t;
        found = true;
        // Gradient of x^2 + z^2 - k^2 * depth^2 gives the outward normal.
        const double px = point.x() - center_.x();
        const double pz = point.z() - center_.z();
        const double depth = y_tip - point.y();
        const Vec3 gradient(px, k2 * depth, pz);
        // At the exact tip the gradient is zero. Point straight up instead.
        outward_normal = gradient.near_zero() ? Vec3(0.0, 1.0, 0.0) : unit_vector(gradient);
    };

    // Expand the side equation into a*t^2 + 2*half_b*t + c = 0.
    const double a = dx * dx + dz * dz - k2 * dy * dy;
    const double half_b = ox * dx + oz * dz + k2 * o_depth * dy;
    const double c = ox * ox + oz * oz - k2 * o_depth * o_depth;

    if (std::fabs(a) < kParallelEpsilon) {
        // The ray is parallel to the cone's slant, so the t^2 term vanishes.
        // The equation is linear with at most one root.
        if (std::fabs(half_b) > kParallelEpsilon) {
            try_side(-c / (2.0 * half_b));
        }
    } else {
        const double discriminant = half_b * half_b - a * c;
        if (discriminant >= 0.0) {
            const double sqrt_discriminant = std::sqrt(discriminant);
            try_side((-half_b - sqrt_discriminant) / a);
            try_side((-half_b + sqrt_discriminant) / a);
        }
    }

    // Base disk at y_base, facing down.
    if (std::fabs(dy) > kParallelEpsilon) {
        const double t = (y_base - ray.origin().y()) / dy;
        if (t > t_min && t < closest) {
            const double px = ox + t * dx;
            const double pz = oz + t * dz;
            if (px * px + pz * pz <= base_radius_ * base_radius_) {
                closest = t;
                found = true;
                outward_normal = Vec3(0.0, -1.0, 0.0);
            }
        }
    }

    if (!found) {
        return false;
    }

    record.t = closest;
    record.point = ray.at(closest);
    record.set_face_normal(ray, outward_normal);
    record.material = material_.get();
    return true;
}

Aabb Cone::bounding_box() const {
    const Vec3 extent(base_radius_, half_height_, base_radius_);
    return Aabb{center_ - extent, center_ + extent};
}

}  // namespace rt
