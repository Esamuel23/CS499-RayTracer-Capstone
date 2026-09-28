//=============================================================================
// File:     test_geometry.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Tests for every shape's intersection routine and the scene list.
//
// Each shape is tested for a direct hit, a miss, the correct normal, a ray
// that starts inside, and the edge cases the code review called out
// (parallel rays, self-intersection near t = 0).
//=============================================================================
#include <memory>
#include <stdexcept>

#include "core/constants.h"
#include "geometry/box.h"
#include "geometry/cone.h"
#include "geometry/cylinder.h"
#include "geometry/hittable_list.h"
#include "geometry/plane.h"
#include "geometry/sphere.h"
#include "materials/material.h"
#include "test_framework.h"

using namespace rt;

namespace {

constexpr double kTolerance = 1e-9;

std::shared_ptr<const Material> gray() {
    return std::make_shared<Lambertian>(Color(0.5, 0.5, 0.5));
}

}  // namespace

//---------------------------------------------------------------- Sphere
RT_TEST(sphere_hit_reports_near_surface_and_outward_normal) {
    const Sphere sphere(Point3(0.0, 0.0, -5.0), 1.0, gray());
    HitRecord record;
    CHECK(sphere.hit(Ray(Point3(0.0, 0.0, 0.0), Vec3(0.0, 0.0, -1.0)), kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.t, 4.0, kTolerance);
    CHECK_NEAR(record.normal.z(), 1.0, kTolerance);
    CHECK(record.front_face);
    CHECK(record.material != nullptr);
}

RT_TEST(sphere_miss) {
    const Sphere sphere(Point3(0.0, 0.0, -5.0), 1.0, gray());
    HitRecord record;
    CHECK(!sphere.hit(Ray(Point3(0.0, 2.0, 0.0), Vec3(0.0, 0.0, -1.0)), kHitEpsilon, kInfinity, record));
}

RT_TEST(sphere_ray_from_inside_hits_back_face) {
    const Sphere sphere(Point3(0.0, 0.0, 0.0), 2.0, gray());
    HitRecord record;
    CHECK(sphere.hit(Ray(Point3(0.0, 0.0, 0.0), Vec3(1.0, 0.0, 0.0)), kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.t, 2.0, kTolerance);
    CHECK(!record.front_face);
    CHECK_NEAR(record.normal.x(), -1.0, kTolerance);  // Faces back toward the ray.
}

RT_TEST(sphere_ignores_self_hit_at_origin) {
    // A bounced ray starting on the surface must not hit that same point.
    const Sphere sphere(Point3(0.0, 0.0, 0.0), 1.0, gray());
    HitRecord record;
    CHECK(!sphere.hit(Ray(Point3(0.0, 1.0, 0.0), Vec3(0.0, 1.0, 0.0)), kHitEpsilon, kInfinity, record));
}

RT_TEST(sphere_rejects_invalid_arguments) {
    CHECK_THROWS_AS(Sphere(Point3(), 0.0, gray()), std::invalid_argument);
    CHECK_THROWS_AS(Sphere(Point3(), 1.0, nullptr), std::invalid_argument);
}

//---------------------------------------------------------------- Box
RT_TEST(box_hit_front_face_normal) {
    const Box box(Point3(0.0, 0.0, 0.0), Vec3(2.0, 2.0, 2.0), gray());
    HitRecord record;
    CHECK(box.hit(Ray(Point3(0.0, 0.0, 5.0), Vec3(0.0, 0.0, -1.0)), kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.t, 4.0, kTolerance);
    CHECK_NEAR(record.normal.z(), 1.0, kTolerance);
    CHECK(record.front_face);
}

RT_TEST(box_hit_top_face_from_above) {
    // Same shape as the original desk top: 6 x 0.3 x 3 centered at y = 3.
    const Box desk(Point3(0.0, 3.0, 0.0), Vec3(6.0, 0.3, 3.0), gray());
    HitRecord record;
    CHECK(desk.hit(Ray(Point3(1.0, 10.0, 0.5), Vec3(0.0, -1.0, 0.0)), kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.point.y(), 3.15, kTolerance);
    CHECK_NEAR(record.normal.y(), 1.0, kTolerance);
}

RT_TEST(box_miss_and_parallel_ray_outside) {
    const Box box(Point3(0.0, 0.0, 0.0), Vec3(2.0, 2.0, 2.0), gray());
    HitRecord record;
    CHECK(!box.hit(Ray(Point3(3.0, 0.0, 5.0), Vec3(0.0, 0.0, -1.0)), kHitEpsilon, kInfinity, record));
    // Parallel to the x slabs and outside them: no hit, no division by zero.
    CHECK(!box.hit(Ray(Point3(1.5, 0.0, 5.0), Vec3(0.0, 0.0, -1.0)), kHitEpsilon, kInfinity, record));
}

RT_TEST(box_ray_from_inside_hits_exit_face) {
    const Box box(Point3(0.0, 0.0, 0.0), Vec3(2.0, 2.0, 2.0), gray());
    HitRecord record;
    CHECK(box.hit(Ray(Point3(0.0, 0.0, 0.0), Vec3(0.0, 1.0, 0.0)), kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.t, 1.0, kTolerance);
    CHECK(!record.front_face);
}

RT_TEST(box_respects_t_max) {
    const Box box(Point3(0.0, 0.0, 0.0), Vec3(2.0, 2.0, 2.0), gray());
    HitRecord record;
    CHECK(!box.hit(Ray(Point3(0.0, 0.0, 5.0), Vec3(0.0, 0.0, -1.0)), kHitEpsilon, 3.0, record));
}

RT_TEST(box_rejects_invalid_arguments) {
    CHECK_THROWS_AS(Box(Point3(), Vec3(1.0, 0.0, 1.0), gray()), std::invalid_argument);
    CHECK_THROWS_AS(Box(Point3(), Vec3(1.0, 1.0, 1.0), nullptr), std::invalid_argument);
}

//---------------------------------------------------------------- Plane
RT_TEST(plane_hit_inside_extents) {
    const Plane ground(Point3(0.0, 0.0, 0.0), 20.0, 20.0, gray());
    HitRecord record;
    CHECK(ground.hit(Ray(Point3(0.0, 5.0, 12.0), Vec3(0.0, -1.0, -1.0)), kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.point.y(), 0.0, kTolerance);
    CHECK_NEAR(record.point.z(), 7.0, kTolerance);
    CHECK_NEAR(record.normal.y(), 1.0, kTolerance);
}

RT_TEST(plane_miss_outside_extents_and_parallel) {
    const Plane ground(Point3(0.0, 0.0, 0.0), 1.0, 1.0, gray());
    HitRecord record;
    CHECK(!ground.hit(Ray(Point3(5.0, 1.0, 0.0), Vec3(0.0, -1.0, 0.0)), kHitEpsilon, kInfinity, record));
    CHECK(!ground.hit(Ray(Point3(0.0, 1.0, 0.0), Vec3(1.0, 0.0, 0.0)), kHitEpsilon, kInfinity, record));
}

RT_TEST(plane_hit_from_below_flips_normal) {
    const Plane ground(Point3(0.0, 0.0, 0.0), 5.0, 5.0, gray());
    HitRecord record;
    CHECK(ground.hit(Ray(Point3(0.0, -1.0, 0.0), Vec3(0.0, 1.0, 0.0)), kHitEpsilon, kInfinity, record));
    CHECK(!record.front_face);
    CHECK_NEAR(record.normal.y(), -1.0, kTolerance);
}

//---------------------------------------------------------------- Cylinder
RT_TEST(cylinder_side_hit) {
    const Cylinder cylinder(Point3(0.0, 0.0, 0.0), 1.0, 2.0, gray());
    HitRecord record;
    CHECK(cylinder.hit(Ray(Point3(5.0, 0.0, 0.0), Vec3(-1.0, 0.0, 0.0)), kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.t, 4.0, kTolerance);
    CHECK_NEAR(record.normal.x(), 1.0, kTolerance);
}

RT_TEST(cylinder_cap_hit_from_above) {
    // Ray runs parallel to the axis, so only the top cap can be hit.
    const Cylinder cylinder(Point3(0.0, 0.0, 0.0), 1.0, 2.0, gray());
    HitRecord record;
    CHECK(cylinder.hit(Ray(Point3(0.5, 5.0, 0.0), Vec3(0.0, -1.0, 0.0)), kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.point.y(), 1.0, kTolerance);
    CHECK_NEAR(record.normal.y(), 1.0, kTolerance);
}

RT_TEST(cylinder_miss_above_and_beside) {
    const Cylinder cylinder(Point3(0.0, 0.0, 0.0), 1.0, 2.0, gray());
    HitRecord record;
    CHECK(!cylinder.hit(Ray(Point3(5.0, 3.0, 0.0), Vec3(-1.0, 0.0, 0.0)), kHitEpsilon, kInfinity, record));
    CHECK(!cylinder.hit(Ray(Point3(5.0, 0.0, 2.0), Vec3(-1.0, 0.0, 0.0)), kHitEpsilon, kInfinity, record));
}

//---------------------------------------------------------------- Cone
RT_TEST(cone_side_hit_at_mid_height) {
    // Radius 1, height 2, center at origin. At y = 0 the radius is 0.5.
    const Cone cone(Point3(0.0, 0.0, 0.0), 1.0, 2.0, gray());
    HitRecord record;
    CHECK(cone.hit(Ray(Point3(5.0, 0.0, 0.0), Vec3(-1.0, 0.0, 0.0)), kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.point.x(), 0.5, 1e-9);
    CHECK(record.normal.x() > 0.0);
    CHECK(record.normal.y() > 0.0);  // Slanted side leans up.
}

RT_TEST(cone_base_hit_from_below) {
    const Cone cone(Point3(0.0, 0.0, 0.0), 1.0, 2.0, gray());
    HitRecord record;
    CHECK(cone.hit(Ray(Point3(0.2, -5.0, 0.0), Vec3(0.0, 1.0, 0.0)), kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.point.y(), -1.0, kTolerance);
    CHECK_NEAR(record.normal.y(), -1.0, kTolerance);
}

RT_TEST(cone_miss_above_tip) {
    // Below the tip the mirror-image cone also solves the equation.
    // The height check must reject it.
    const Cone cone(Point3(0.0, 0.0, 0.0), 1.0, 2.0, gray());
    HitRecord record;
    CHECK(!cone.hit(Ray(Point3(5.0, 1.5, 0.0), Vec3(-1.0, 0.0, 0.0)), kHitEpsilon, kInfinity, record));
}

RT_TEST(cone_ray_parallel_to_slant) {
    // Slope 1 cone (radius 1, height 1). A direction of (-1, -1, 0) makes
    // a = 0, which takes the linear branch instead of the quadratic.
    const Cone cone(Point3(0.0, 0.0, 0.0), 1.0, 1.0, gray());
    HitRecord record;
    CHECK(cone.hit(Ray(Point3(2.5, 2.0, 0.0), Vec3(-1.0, -1.0, 0.0)), kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.t, 2.0, kTolerance);
    CHECK_NEAR(record.point.x(), 0.5, kTolerance);
}

//---------------------------------------------------------------- List
RT_TEST(hittable_list_returns_closest_hit) {
    HittableList world;
    world.add(std::make_unique<Sphere>(Point3(0.0, 0.0, -10.0), 1.0, gray()));
    world.add(std::make_unique<Sphere>(Point3(0.0, 0.0, -5.0), 1.0, gray()));
    HitRecord record;
    CHECK(world.hit(Ray(Point3(0.0, 0.0, 0.0), Vec3(0.0, 0.0, -1.0)), kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.t, 4.0, kTolerance);
    CHECK(world.size() == 2);
}

RT_TEST(hittable_list_rejects_null_and_empty_box) {
    HittableList world;
    CHECK_THROWS_AS(world.add(nullptr), std::invalid_argument);
    CHECK_THROWS_AS(world.bounding_box(), std::logic_error);
}

RT_TEST(bounding_boxes_contain_their_shapes) {
    const Cylinder cylinder(Point3(1.0, 2.0, 3.0), 0.5, 2.0, gray());
    const Aabb box = cylinder.bounding_box();
    CHECK_NEAR(box.min.y(), 1.0, kTolerance);
    CHECK_NEAR(box.max.x(), 1.5, kTolerance);
}
