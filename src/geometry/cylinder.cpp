//=============================================================================
// File:     cylinder.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Ray intersection with a capped, upright cylinder.
//=============================================================================
#include "geometry/cylinder.h"

#include <cmath>
#include <stdexcept>
#include <utility>

#include "core/constants.h"

namespace rt {

Cylinder::Cylinder(const Point3& center, double radius, double height,
                   std::shared_ptr<const Material> material)
    : center_(center),
      radius_(radius),
      half_height_(0.5 * height),
      material_(std::move(material)) {
    if (!(radius_ > 0.0 && half_height_ > 0.0)) {
        throw std::invalid_argument("Cylinder radius and height must be positive");
    }
    if (!material_) {
        throw std::invalid_argument("Cylinder requires a material");
    }
}

bool Cylinder::hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const {
    const double y_low = center_.y() - half_height_;
    const double y_high = center_.y() + half_height_;

    // Work relative to the cylinder axis.
    const double ox = ray.origin().x() - center_.x();
    const double oz = ray.origin().z() - center_.z();
    const double dx = ray.direction().x();
    const double dy = ray.direction().y();
    const double dz = ray.direction().z();

    // "closest" shrinks every time a nearer valid hit is found, so the
    // side and both caps compete and the nearest one wins.
    double closest = t_max;
    bool found = false;
    Vec3 outward_normal;

    // Curved side: (ox + t*dx)^2 + (oz + t*dz)^2 = r^2.
    // If a is zero the ray runs parallel to the axis and can only hit a cap.
    const double a = dx * dx + dz * dz;
    if (a > kParallelEpsilon) {
        const double half_b = ox * dx + oz * dz;
        const double c = ox * ox + oz * oz - radius_ * radius_;
        const double discriminant = half_b * half_b - a * c;
        if (discriminant >= 0.0) {
            const double sqrt_discriminant = std::sqrt(discriminant);
            const double roots[2] = {(-half_b - sqrt_discriminant) / a,
                                     (-half_b + sqrt_discriminant) / a};
            for (const double root : roots) {
                if (root <= t_min || root >= closest) {
                    continue;
                }
                // The infinite cylinder is hit. Keep it only between the caps.
                const double y = ray.origin().y() + root * dy;
                if (y < y_low || y > y_high) {
                    continue;
                }
                closest = root;
                found = true;
                const Point3 point = ray.at(root);
                outward_normal = Vec3(point.x() - center_.x(), 0.0, point.z() - center_.z()) / radius_;
            }
        }
    }

    // Caps: disks at y_low (facing down) and y_high (facing up).
    // A ray parallel to the caps cannot hit them.
    if (std::fabs(dy) > kParallelEpsilon) {
        const double cap_heights[2] = {y_low, y_high};
        const double cap_signs[2] = {-1.0, 1.0};
        for (int i = 0; i < 2; ++i) {
            const double t = (cap_heights[i] - ray.origin().y()) / dy;
            if (t <= t_min || t >= closest) {
                continue;
            }
            const double px = ox + t * dx;
            const double pz = oz + t * dz;
            if (px * px + pz * pz <= radius_ * radius_) {
                closest = t;
                found = true;
                outward_normal = Vec3(0.0, cap_signs[i], 0.0);
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

Aabb Cylinder::bounding_box() const {
    const Vec3 extent(radius_, half_height_, radius_);
    return Aabb{center_ - extent, center_ + extent};
}

}  // namespace rt
