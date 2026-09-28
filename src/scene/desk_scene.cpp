//=============================================================================
// File:     desk_scene.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Builds the desk scene and the showcase scene.
//=============================================================================
#include "scene/desk_scene.h"

#include "geometry/box.h"
#include "geometry/cone.h"
#include "geometry/cylinder.h"
#include "geometry/plane.h"
#include "geometry/sphere.h"
#include "materials/material.h"

namespace rt::scene {
namespace {

// Showcase props. They sit on the desk top, so their heights are measured
// from the top surface.
constexpr double kDeskSurfaceY = kDeskTopCenter.y() + kDeskTopSize.y() / 2.0;

constexpr double kSphereRadius = 0.5;
constexpr Point3 kMirrorSphereCenter{-1.7, kDeskSurfaceY + kSphereRadius, 0.2};
constexpr Point3 kGlassSphereCenter{-0.4, kDeskSurfaceY + kSphereRadius, 0.6};
constexpr Color kMirrorColor{0.85, 0.85, 0.9};
constexpr double kMirrorFuzz = 0.02;
constexpr double kGlassIndex = 1.5;  // Typical window glass.

constexpr double kMugRadius = 0.35;
constexpr double kMugHeight = 0.8;
constexpr Point3 kMugCenter{0.9, kDeskSurfaceY + kMugHeight / 2.0, -0.4};
constexpr Color kMugColor{0.75, 0.2, 0.15};

constexpr double kConeRadius = 0.4;
constexpr double kConeHeight = 1.0;
constexpr Point3 kConeCenter{2.1, kDeskSurfaceY + kConeHeight / 2.0, 0.4};
constexpr Color kConeColor{0.9, 0.7, 0.2};

// Adds the parts shared by both scenes: ground, desk top, and legs.
void add_desk(HittableList& world) {
    // One material per color, shared by every shape that uses it.
    const auto ground = std::make_shared<Lambertian>(kGroundColor);
    const auto desk_top = std::make_shared<Lambertian>(kDeskTopColor);
    const auto leg = std::make_shared<Lambertian>(kLegColor);

    world.add(std::make_unique<Plane>(kGroundCenter, kGroundHalfExtent, kGroundHalfExtent, ground));
    world.add(std::make_unique<Box>(kDeskTopCenter, kDeskTopSize, desk_top));
    for (const Point3& position : leg_positions()) {
        world.add(std::make_unique<Box>(position, kLegSize, leg));
    }
}

}  // namespace

std::array<Point3, kLegCount> leg_positions() {
    const double inset_x = kDeskTopSize.x() / 2.0 - kLegSize.x();
    const double inset_z = kDeskTopSize.z() / 2.0 - kLegSize.z();
    const double center_y = kLegSize.y() / 2.0;
    return {
        Point3(-inset_x, center_y, -inset_z),  // Front left.
        Point3(inset_x, center_y, -inset_z),   // Front right.
        Point3(-inset_x, center_y, inset_z),   // Back left.
        Point3(inset_x, center_y, inset_z),    // Back right.
    };
}

CameraSettings original_camera(double aspect_ratio) {
    return CameraSettings{kCameraPosition, kCameraFront, kCameraUp, kCameraFovDegrees,
                          aspect_ratio};
}

CameraSettings closeup_camera(double aspect_ratio) {
    return CameraSettings{kCloseupPosition, kCloseupFront, kCameraUp, kCloseupFovDegrees,
                          aspect_ratio};
}

std::unique_ptr<HittableList> build_desk_scene() {
    auto world = std::make_unique<HittableList>();
    add_desk(*world);
    return world;
}

std::unique_ptr<HittableList> build_showcase_scene() {
    auto world = build_desk_scene();

    world->add(std::make_unique<Sphere>(kMirrorSphereCenter, kSphereRadius,
                                        std::make_shared<Metal>(kMirrorColor, kMirrorFuzz)));
    world->add(std::make_unique<Sphere>(kGlassSphereCenter, kSphereRadius,
                                        std::make_shared<Dielectric>(kGlassIndex)));
    world->add(std::make_unique<Cylinder>(kMugCenter, kMugRadius, kMugHeight,
                                          std::make_shared<Lambertian>(kMugColor)));
    world->add(std::make_unique<Cone>(kConeCenter, kConeRadius, kConeHeight,
                                      std::make_shared<Lambertian>(kConeColor)));
    return world;
}

}  // namespace rt::scene
