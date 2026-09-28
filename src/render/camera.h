//=============================================================================
// File:     camera.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Pinhole camera that turns image coordinates into rays.
//
// Design notes:
//   The original ViewManager fed Position, Front, Up, and Zoom into
//   glm::lookAt and glm::perspective. CameraSettings keeps those same four
//   values, so the ray-traced image frames the scene the same way.
//   The aspect ratio now comes from the output image size. The original
//   fixed it to the startup window size, which the code review flagged.
//=============================================================================
#pragma once

#include "core/ray.h"
#include "core/vec3.h"

namespace rt {

struct CameraSettings {
    Point3 position;
    Vec3 front;                   // Direction the camera looks.
    Vec3 up;                      // Approximate up direction.
    double vertical_fov_degrees;  // Same meaning as the original Zoom.
    double aspect_ratio;          // Image width / image height.
};

class Camera {
public:
    // Throws std::invalid_argument if front is zero, up is parallel to
    // front, the field of view is outside (0, 180), or aspect_ratio <= 0.
    explicit Camera(const CameraSettings& settings);

    // s and t are in [0, 1]. (0, 0) is the bottom-left of the image and
    // (1, 1) is the top-right.
    Ray generate_ray(double s, double t) const;

private:
    Point3 origin_;
    Point3 lower_left_corner_;
    Vec3 horizontal_;
    Vec3 vertical_;
};

}  // namespace rt
