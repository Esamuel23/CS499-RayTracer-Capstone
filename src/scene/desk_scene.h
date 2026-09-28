//=============================================================================
// File:     desk_scene.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  The CS 330 desk scene, rebuilt as Hittables and Materials.
//
// Design notes:
//   Every number here comes from the original SceneManager::RenderScene and
//   ViewManager constructor. The code review found them written as unnamed
//   literals, and found that the leg positions only matched the desk by
//   coincidence. They are now named constants, and the leg positions are
//   computed from the desk and leg sizes.
//=============================================================================
#pragma once

#include <array>
#include <memory>

#include "core/vec3.h"
#include "geometry/hittable_list.h"
#include "render/camera.h"

namespace rt::scene {

// Ground plane. The course plane mesh spans -1 to 1 and was scaled by 20.
constexpr double kGroundHalfExtent = 20.0;
constexpr Point3 kGroundCenter{0.0, 0.0, 0.0};
constexpr Color kGroundColor{0.3, 0.8, 0.3};

// Desk surface: a flat box raised above the ground.
constexpr Vec3 kDeskTopSize{6.0, 0.3, 3.0};  // Width, thickness, depth.
constexpr Point3 kDeskTopCenter{0.0, 3.0, 0.0};
constexpr Color kDeskTopColor{0.2, 0.2, 0.2};

// Desk legs: thin, tall boxes under each corner.
constexpr int kLegCount = 4;
constexpr Vec3 kLegSize{0.3, 3.0, 0.3};
constexpr Color kLegColor{0.1, 0.1, 0.1};

// Camera values from the original ViewManager constructor.
constexpr Point3 kCameraPosition{0.0, 5.0, 12.0};
constexpr Vec3 kCameraFront{0.0, -0.5, -2.0};
constexpr Vec3 kCameraUp{0.0, 1.0, 0.0};
constexpr double kCameraFovDegrees = 80.0;

// Closer view added for the enhancement, framed on the desk top so the
// reflection and refraction are easy to see.
constexpr Point3 kCloseupPosition{0.0, 5.2, 6.5};
constexpr Vec3 kCloseupFront{0.0, -1.8, -6.5};
constexpr double kCloseupFovDegrees = 50.0;

// Leg centers, derived from the sizes above. Each leg is inset by one leg
// width from the desk edge and stands on the ground. This reproduces the
// original literals (x = +/-2.7, y = 1.5, z = +/-1.2) exactly.
std::array<Point3, kLegCount> leg_positions();

// Original camera, with the aspect ratio taken from the output image.
CameraSettings original_camera(double aspect_ratio);

// Close-up camera on the desk top, same aspect ratio rule.
CameraSettings closeup_camera(double aspect_ratio);

// Faithful rebuild of the CS 330 scene: ground plane, desk top, four legs.
std::unique_ptr<HittableList> build_desk_scene();

// The desk scene plus one object of each remaining shape and material
// type. Shows what the new design supports that the original did not:
// reflection, refraction, and curved analytic surfaces.
std::unique_ptr<HittableList> build_showcase_scene();

}  // namespace rt::scene
