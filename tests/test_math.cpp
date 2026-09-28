//=============================================================================
// File:     test_math.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Tests for Vec3, Ray, and the bounding box slab test.
//=============================================================================
#include "core/aabb.h"
#include "core/constants.h"
#include "core/random.h"
#include "core/ray.h"
#include "core/vec3.h"
#include "test_framework.h"

using namespace rt;

namespace {
constexpr double kTolerance = 1e-9;
}

RT_TEST(vec3_arithmetic) {
    const Vec3 a(1.0, 2.0, 3.0);
    const Vec3 b(4.0, 5.0, 6.0);
    const Vec3 sum = a + b;
    CHECK_NEAR(sum.x(), 5.0, kTolerance);
    CHECK_NEAR(sum.y(), 7.0, kTolerance);
    CHECK_NEAR(sum.z(), 9.0, kTolerance);
    CHECK_NEAR(dot(a, b), 32.0, kTolerance);
    CHECK_NEAR((2.0 * a).z(), 6.0, kTolerance);
    CHECK_NEAR((a / 2.0).x(), 0.5, kTolerance);
}

RT_TEST(vec3_cross_is_perpendicular) {
    const Vec3 x(1.0, 0.0, 0.0);
    const Vec3 y(0.0, 1.0, 0.0);
    const Vec3 z = cross(x, y);
    CHECK_NEAR(z.z(), 1.0, kTolerance);
    CHECK_NEAR(dot(z, x), 0.0, kTolerance);
    CHECK_NEAR(dot(z, y), 0.0, kTolerance);
}

RT_TEST(vec3_unit_vector_has_length_one) {
    CHECK_NEAR(unit_vector(Vec3(3.0, 4.0, 12.0)).length(), 1.0, kTolerance);
}

RT_TEST(vec3_reflect_flips_normal_component) {
    const Vec3 reflected = reflect(Vec3(1.0, -1.0, 0.0), Vec3(0.0, 1.0, 0.0));
    CHECK_NEAR(reflected.x(), 1.0, kTolerance);
    CHECK_NEAR(reflected.y(), 1.0, kTolerance);
}

RT_TEST(vec3_refract_straight_through_at_normal_incidence) {
    // A ray hitting glass head-on does not bend.
    const Vec3 refracted = refract(Vec3(0.0, -1.0, 0.0), Vec3(0.0, 1.0, 0.0), 1.0 / 1.5);
    CHECK_NEAR(refracted.x(), 0.0, kTolerance);
    CHECK_NEAR(refracted.y(), -1.0, kTolerance);
}

RT_TEST(ray_at_moves_along_direction) {
    const Ray ray(Point3(1.0, 0.0, 0.0), Vec3(0.0, 2.0, 0.0));
    CHECK_NEAR(ray.at(1.5).y(), 3.0, kTolerance);
    CHECK_NEAR(ray.at(1.5).x(), 1.0, kTolerance);
}

RT_TEST(rng_is_reproducible_and_in_range) {
    Rng first(42);
    Rng second(42);
    for (int i = 0; i < 1000; ++i) {
        const double value = first.next();
        CHECK(value >= 0.0 && value < 1.0);
        CHECK_NEAR(value, second.next(), 0.0);
    }
    CHECK_NEAR(first.unit_vector().length(), 1.0, 1e-9);
}

RT_TEST(aabb_hit_and_miss) {
    const Aabb box{Point3(-1.0, -1.0, -1.0), Point3(1.0, 1.0, 1.0)};
    CHECK(box.hit(Ray(Point3(0.0, 0.0, -5.0), Vec3(0.0, 0.0, 1.0)), kHitEpsilon, kInfinity));
    CHECK(!box.hit(Ray(Point3(3.0, 0.0, -5.0), Vec3(0.0, 0.0, 1.0)), kHitEpsilon, kInfinity));
    // Box is behind the ray.
    CHECK(!box.hit(Ray(Point3(0.0, 0.0, 5.0), Vec3(0.0, 0.0, 1.0)), kHitEpsilon, kInfinity));
}

RT_TEST(aabb_parallel_ray_does_not_divide_by_zero) {
    // Direction has zero x and y. Inside the slabs: hit. Outside: miss.
    const Aabb box{Point3(-1.0, -1.0, -1.0), Point3(1.0, 1.0, 1.0)};
    CHECK(box.hit(Ray(Point3(0.5, 0.5, -5.0), Vec3(0.0, 0.0, 1.0)), kHitEpsilon, kInfinity));
    CHECK(!box.hit(Ray(Point3(1.5, 0.5, -5.0), Vec3(0.0, 0.0, 1.0)), kHitEpsilon, kInfinity));
}

RT_TEST(aabb_surrounding_contains_both) {
    const Aabb a{Point3(0.0, 0.0, 0.0), Point3(1.0, 1.0, 1.0)};
    const Aabb b{Point3(-2.0, 0.5, 0.0), Point3(0.5, 3.0, 0.5)};
    const Aabb both = Aabb::surrounding(a, b);
    CHECK_NEAR(both.min.x(), -2.0, kTolerance);
    CHECK_NEAR(both.max.y(), 3.0, kTolerance);
    CHECK_NEAR(both.max.z(), 1.0, kTolerance);
}
