//=============================================================================
// File:     camera.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Builds the camera basis and the virtual viewport.
//=============================================================================
#include "render/camera.h"

#include <cmath>
#include <stdexcept>

#include "core/constants.h"

namespace rt {

Camera::Camera(const CameraSettings& settings) : origin_(settings.position) {
    if (settings.front.near_zero()) {
        throw std::invalid_argument("Camera front vector must not be zero");
    }
    if (!(settings.vertical_fov_degrees > 0.0 && settings.vertical_fov_degrees < 180.0)) {
        throw std::invalid_argument("Camera field of view must be between 0 and 180 degrees");
    }
    if (!(settings.aspect_ratio > 0.0)) {
        throw std::invalid_argument("Camera aspect ratio must be positive");
    }

    // Orthonormal basis, the same one glm::lookAt builds.
    // w points backward, u points right, v points up.
    const Vec3 w = unit_vector(-settings.front);
    const Vec3 right = cross(settings.up, w);
    if (right.near_zero()) {
        throw std::invalid_argument("Camera up vector must not be parallel to front");
    }
    const Vec3 u = unit_vector(right);
    const Vec3 v = cross(w, u);

    // Viewport one unit in front of the camera, sized by the field of view.
    const double theta = degrees_to_radians(settings.vertical_fov_degrees);
    const double viewport_height = 2.0 * std::tan(theta / 2.0);
    const double viewport_width = settings.aspect_ratio * viewport_height;

    horizontal_ = viewport_width * u;
    vertical_ = viewport_height * v;
    lower_left_corner_ = origin_ - horizontal_ / 2.0 - vertical_ / 2.0 - w;
}

Ray Camera::generate_ray(double s, double t) const {
    return Ray(origin_, lower_left_corner_ + s * horizontal_ + t * vertical_ - origin_);
}

}  // namespace rt
