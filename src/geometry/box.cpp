//=============================================================================
// File:     box.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Ray-box intersection that also reports which face was hit.
//=============================================================================
#include "geometry/box.h"

#include <cmath>
#include <stdexcept>
#include <utility>

#include "core/constants.h"

namespace rt {
namespace {

// Unit vector along one axis, with a sign (+1 or -1).
Vec3 axis_normal(int axis, double sign) {
    if (axis == 0) {
        return Vec3(sign, 0.0, 0.0);
    }
    if (axis == 1) {
        return Vec3(0.0, sign, 0.0);
    }
    return Vec3(0.0, 0.0, sign);
}

}  // namespace

Box::Box(const Point3& center, const Vec3& size, std::shared_ptr<const Material> material)
    : min_(center - 0.5 * size), max_(center + 0.5 * size), material_(std::move(material)) {
    if (!(size.x() > 0.0 && size.y() > 0.0 && size.z() > 0.0)) {
        throw std::invalid_argument("Box size must be positive on every axis");
    }
    if (!material_) {
        throw std::invalid_argument("Box requires a material");
    }
}

bool Box::hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const {
    // Track where the ray enters and leaves the box, and on which axis.
    // The axis tells us the face, and the face gives the normal.
    double t_enter = -kInfinity;
    double t_exit = kInfinity;
    int enter_axis = 0;
    int exit_axis = 0;
    double enter_sign = -1.0;
    double exit_sign = 1.0;

    for (int axis = 0; axis < 3; ++axis) {
        const double origin = ray.origin()[axis];
        const double direction = ray.direction()[axis];

        // Parallel to this pair of faces. No division. The ray is either
        // always between the faces or never between them.
        if (std::fabs(direction) < kParallelEpsilon) {
            if (origin < min_[axis] || origin > max_[axis]) {
                return false;
            }
            continue;
        }

        const double inverse = 1.0 / direction;
        const double t_low = (min_[axis] - origin) * inverse;
        const double t_high = (max_[axis] - origin) * inverse;

        // Moving in +axis: enter through the low face, leave through the high.
        // Moving in -axis: the reverse.
        const bool positive = direction > 0.0;
        const double t_near = positive ? t_low : t_high;
        const double t_far = positive ? t_high : t_low;

        if (t_near > t_enter) {
            t_enter = t_near;
            enter_axis = axis;
            enter_sign = positive ? -1.0 : 1.0;
        }
        if (t_far < t_exit) {
            t_exit = t_far;
            exit_axis = axis;
            exit_sign = positive ? 1.0 : -1.0;
        }
        if (t_enter > t_exit) {
            return false;
        }
    }

    // Common case first: the ray starts outside and enters the box.
    // Otherwise the ray starts inside (glass or a bounce) and we use the exit.
    double t = 0.0;
    Vec3 outward_normal;
    if (t_enter > t_min && t_enter < t_max) {
        t = t_enter;
        outward_normal = axis_normal(enter_axis, enter_sign);
    } else if (t_exit > t_min && t_exit < t_max) {
        t = t_exit;
        outward_normal = axis_normal(exit_axis, exit_sign);
    } else {
        return false;
    }

    record.t = t;
    record.point = ray.at(t);
    record.set_face_normal(ray, outward_normal);
    record.material = material_.get();
    return true;
}

Aabb Box::bounding_box() const { return Aabb{min_, max_}; }

}  // namespace rt
