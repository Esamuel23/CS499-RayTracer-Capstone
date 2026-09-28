//=============================================================================
// File:     sphere.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Ray-sphere intersection by solving a quadratic in t.
//=============================================================================
#include "geometry/sphere.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace rt {

Sphere::Sphere(const Point3& center, double radius, std::shared_ptr<const Material> material)
    : center_(center), radius_(radius), material_(std::move(material)) {
    if (!(radius_ > 0.0)) {
        throw std::invalid_argument("Sphere radius must be positive");
    }
    if (!material_) {
        throw std::invalid_argument("Sphere requires a material");
    }
}

bool Sphere::hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const {
    // Substitute P(t) into |P - C|^2 = r^2 and solve a*t^2 - 2*h*t + c = 0.
    // Using h (half of b) removes a factor of 2 from every term.
    const Vec3 to_center = center_ - ray.origin();
    const double a = ray.direction().length_squared();
    const double h = dot(ray.direction(), to_center);
    const double c = to_center.length_squared() - radius_ * radius_;

    const double discriminant = h * h - a * c;
    if (discriminant < 0.0) {
        return false;  // The ray line never touches the sphere.
    }

    // Try the nearer root first. If it is out of range, try the farther
    // one. The farther root is the exit point when the ray starts inside.
    const double sqrt_discriminant = std::sqrt(discriminant);
    double root = (h - sqrt_discriminant) / a;
    if (root <= t_min || root >= t_max) {
        root = (h + sqrt_discriminant) / a;
        if (root <= t_min || root >= t_max) {
            return false;
        }
    }

    record.t = root;
    record.point = ray.at(root);
    record.set_face_normal(ray, (record.point - center_) / radius_);
    record.material = material_.get();
    return true;
}

Aabb Sphere::bounding_box() const {
    const Vec3 extent(radius_, radius_, radius_);
    return Aabb{center_ - extent, center_ + extent};
}

}  // namespace rt
