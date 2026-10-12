//=============================================================================
// File:     test_accel.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     October 2026
// Purpose:  Tests for the bounding volume hierarchy (Enhancement Two) and
//           the --accel command-line option.
//
// Design notes:
//   HittableList is the trusted baseline: it is simple enough to check by
//   reading it. Most tests here build the same shapes twice, once into a
//   list and once into a Bvh, and require the two to give exactly the same
//   answer. Every sphere gets its own material, so the material pointer in
//   a hit record says which sphere was hit. If the BVH ever returned a
//   different sphere than the list, the test would see it.
//   Random scenes use fixed seeds so any failure can be reproduced.
//=============================================================================
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "accel/bvh.h"
#include "app/options.h"
#include "core/constants.h"
#include "core/random.h"
#include "geometry/hittable_list.h"
#include "geometry/sphere.h"
#include "materials/material.h"
#include "render/camera.h"
#include "render/renderer.h"
#include "scene/desk_scene.h"
#include "test_framework.h"

using namespace rt;

namespace {

// Fixed seeds so a failure reproduces exactly.
constexpr std::uint64_t kSceneSeed = 499;
constexpr std::uint64_t kRaySeed = 2026;

constexpr int kEqualitySphereCount = 1000;
constexpr int kEqualityRayCount = 10000;
constexpr int kLargeSphereCount = 10000;
constexpr int kDegenerateRayCount = 200;
constexpr double kSceneHalfExtent = 10.0;  // Spheres are centered in [-10, 10]^3.
constexpr double kRayOriginHalfExtent = 15.0;
constexpr double kMinRadius = 0.1;
constexpr double kMaxRadius = 1.0;

struct SphereSpec {
    Point3 center;
    double radius;
};

// The geometry and the materials are kept apart from the shapes, so the
// exact same scene can be built twice (once for a list, once for a BVH)
// with shared material pointers.
struct SceneSpec {
    std::vector<SphereSpec> spheres;
    std::vector<std::shared_ptr<const Material>> materials;
};

SceneSpec random_scene(int count, std::uint64_t seed) {
    Rng rng(seed);
    SceneSpec scene;
    for (int i = 0; i < count; ++i) {
        const Point3 center(rng.next(-kSceneHalfExtent, kSceneHalfExtent),
                            rng.next(-kSceneHalfExtent, kSceneHalfExtent),
                            rng.next(-kSceneHalfExtent, kSceneHalfExtent));
        scene.spheres.push_back(SphereSpec{center, rng.next(kMinRadius, kMaxRadius)});
        scene.materials.push_back(std::make_shared<Lambertian>(Color(rng.next(), rng.next(), rng.next())));
    }
    return scene;
}

std::vector<std::unique_ptr<Hittable>> make_objects(const SceneSpec& scene) {
    std::vector<std::unique_ptr<Hittable>> objects;
    for (std::size_t i = 0; i < scene.spheres.size(); ++i) {
        objects.push_back(std::make_unique<Sphere>(scene.spheres[i].center, scene.spheres[i].radius,
                                                   scene.materials[i]));
    }
    return objects;
}

std::unique_ptr<HittableList> make_list(const SceneSpec& scene) {
    auto list = std::make_unique<HittableList>();
    for (auto& object : make_objects(scene)) {
        list->add(std::move(object));
    }
    return list;
}

// Half the rays point in a random direction. The other half aim at a
// random point inside the scene so most of them hit something. A quarter
// get a finite t_max to exercise the upper bound as well.
struct TestRay {
    Ray ray;
    double t_max;
};

std::vector<TestRay> random_rays(int count, std::uint64_t seed) {
    Rng rng(seed);
    std::vector<TestRay> rays;
    for (int i = 0; i < count; ++i) {
        const Point3 origin(rng.next(-kRayOriginHalfExtent, kRayOriginHalfExtent),
                            rng.next(-kRayOriginHalfExtent, kRayOriginHalfExtent),
                            rng.next(-kRayOriginHalfExtent, kRayOriginHalfExtent));
        Vec3 direction = rng.unit_vector();
        if (i % 2 == 0) {
            const Point3 target(rng.next(-kSceneHalfExtent, kSceneHalfExtent),
                                rng.next(-kSceneHalfExtent, kSceneHalfExtent),
                                rng.next(-kSceneHalfExtent, kSceneHalfExtent));
            if (!(target - origin).near_zero()) {
                direction = target - origin;
            }
        }
        const double t_max = (i % 4 == 1) ? rng.next(1.0, 30.0) : kInfinity;
        rays.push_back(TestRay{Ray(origin, direction), t_max});
    }
    return rays;
}

// True when both structures agree on hit or miss and, on a hit, on every
// field of the record. Values must match exactly (tolerance 0): the same
// shape computes the same numbers no matter which structure asked.
bool same_answer(const Hittable& expected, const Hittable& actual, const Ray& ray,
                 double t_max, bool& was_hit) {
    HitRecord want;
    HitRecord got;
    const bool want_hit = expected.hit(ray, kHitEpsilon, t_max, want);
    const bool got_hit = actual.hit(ray, kHitEpsilon, t_max, got);
    was_hit = want_hit;
    if (want_hit != got_hit) {
        return false;
    }
    if (!want_hit) {
        return true;
    }
    return want.t == got.t && want.point.x() == got.point.x() && want.point.y() == got.point.y() &&
           want.point.z() == got.point.z() && want.normal.x() == got.normal.x() &&
           want.normal.y() == got.normal.y() && want.normal.z() == got.normal.z() &&
           want.front_face == got.front_face && want.material == got.material;
}

// A shape with whatever box the test needs. It is never hit. Used to feed
// the builder degenerate boxes that no real shape in the project produces.
class BoxOnly final : public Hittable {
public:
    explicit BoxOnly(const Aabb& box) : box_(box) {}
    bool hit(const Ray&, double, double, HitRecord&) const override { return false; }
    Aabb bounding_box() const override { return box_; }

private:
    Aabb box_;
};

bool box_inside(const Aabb& inner, const Aabb& outer) {
    for (int axis = 0; axis < 3; ++axis) {
        if (inner.min[axis] < outer.min[axis] || inner.max[axis] > outer.max[axis]) {
            return false;
        }
    }
    return true;
}

// Runs the list-versus-BVH comparison for one build strategy.
void check_random_scene_matches_list(BvhSplit split) {
    const SceneSpec scene = random_scene(kEqualitySphereCount, kSceneSeed);
    const auto list = make_list(scene);
    const Bvh bvh(make_objects(scene), split);

    int mismatches = 0;
    int hits = 0;
    for (const TestRay& test : random_rays(kEqualityRayCount, kRaySeed)) {
        bool was_hit = false;
        if (!same_answer(*list, bvh, test.ray, test.t_max, was_hit)) {
            ++mismatches;
        }
        hits += was_hit ? 1 : 0;
    }
    CHECK(mismatches == 0);
    // Guard against a test that passes because every ray missed.
    CHECK(hits > kEqualityRayCount / 4);
    CHECK(hits < kEqualityRayCount);
}

}  // namespace

//---------------------------------------------------------------- 1. Input
RT_TEST(bvh_rejects_empty_and_null_input) {
    CHECK_THROWS_AS(Bvh(std::vector<std::unique_ptr<Hittable>>{}), std::invalid_argument);

    std::vector<std::unique_ptr<Hittable>> with_null;
    with_null.push_back(nullptr);
    CHECK_THROWS_AS(Bvh(std::move(with_null)), std::invalid_argument);
}

//---------------------------------------------------------------- 2. Base case
RT_TEST(bvh_single_sphere_matches_list) {
    const SceneSpec scene{{SphereSpec{Point3(0.0, 0.0, -5.0), 1.0}},
                          {std::make_shared<Lambertian>(Color(0.5, 0.5, 0.5))}};
    const auto list = make_list(scene);
    const Bvh bvh(make_objects(scene));

    CHECK(bvh.nodes().size() == 1);
    CHECK(bvh.nodes().front().is_leaf());
    CHECK(bvh.depth() == 0);

    HitRecord from_list;
    HitRecord from_bvh;
    const Ray ray(Point3(0.0, 0.0, 0.0), Vec3(0.0, 0.0, -1.0));
    CHECK(list->hit(ray, kHitEpsilon, kInfinity, from_list));
    CHECK(bvh.hit(ray, kHitEpsilon, kInfinity, from_bvh));
    CHECK_NEAR(from_bvh.t, from_list.t, 0.0);
    CHECK_NEAR(from_bvh.t, 4.0, 1e-12);
    CHECK_NEAR(from_bvh.point.z(), from_list.point.z(), 0.0);
    CHECK_NEAR(from_bvh.normal.z(), from_list.normal.z(), 0.0);
    CHECK(from_bvh.material == from_list.material);
}

//---------------------------------------------------------------- 3. Equality
RT_TEST(bvh_sah_matches_list_on_random_scene) {
    check_random_scene_matches_list(BvhSplit::Sah);
}

RT_TEST(bvh_median_matches_list_on_random_scene) {
    check_random_scene_matches_list(BvhSplit::Median);
}

//---------------------------------------------------------------- 4. Miss contract
RT_TEST(bvh_root_miss_leaves_record_untouched) {
    const Bvh bvh(make_objects(random_scene(50, kSceneSeed)));

    HitRecord record;
    record.t = 123.0;
    record.point = Point3(7.0, 8.0, 9.0);
    record.front_face = true;

    // Starts far outside the scene and points away from it.
    const Ray away(Point3(100.0, 100.0, 100.0), Vec3(1.0, 1.0, 1.0));
    CHECK(!bvh.hit(away, kHitEpsilon, kInfinity, record));
    CHECK_NEAR(record.t, 123.0, 0.0);
    CHECK_NEAR(record.point.y(), 8.0, 0.0);
    CHECK(record.front_face);
    CHECK(record.material == nullptr);
}

//---------------------------------------------------------------- 5-7. Build invariants
RT_TEST(bvh_root_bounds_equal_union_of_shapes) {
    const SceneSpec scene = random_scene(kEqualitySphereCount, kSceneSeed);
    const Aabb expected = make_list(scene)->bounding_box();
    const Bvh bvh(make_objects(scene));
    const Aabb root = bvh.bounding_box();
    for (int axis = 0; axis < 3; ++axis) {
        CHECK_NEAR(root.min[axis], expected.min[axis], 0.0);
        CHECK_NEAR(root.max[axis], expected.max[axis], 0.0);
    }
}

RT_TEST(bvh_children_are_inside_parents) {
    for (const BvhSplit split : {BvhSplit::Sah, BvhSplit::Median}) {
        const Bvh bvh(make_objects(random_scene(kEqualitySphereCount, kSceneSeed)), split);
        const auto& nodes = bvh.nodes();
        int bad_children = 0;
        int bad_leaves = 0;
        for (const Bvh::Node& node : nodes) {
            if (node.is_leaf()) {
                // Every shape in a leaf fits inside the leaf box.
                for (std::uint32_t i = node.left_or_first; i < node.left_or_first + node.count; ++i) {
                    bad_leaves += box_inside(bvh.primitive(i).bounding_box(), node.bounds) ? 0 : 1;
                }
                continue;
            }
            CHECK(node.left_or_first + 1 < nodes.size());
            bad_children += box_inside(nodes[node.left_or_first].bounds, node.bounds) ? 0 : 1;
            bad_children += box_inside(nodes[node.left_or_first + 1].bounds, node.bounds) ? 0 : 1;
        }
        CHECK(bad_children == 0);
        CHECK(bad_leaves == 0);
    }
}

RT_TEST(bvh_every_shape_in_exactly_one_leaf) {
    for (const BvhSplit split : {BvhSplit::Sah, BvhSplit::Median}) {
        auto objects = make_objects(random_scene(kEqualitySphereCount, kSceneSeed));
        std::unordered_set<const Hittable*> originals;
        for (const auto& object : objects) {
            originals.insert(object.get());
        }

        const Bvh bvh(std::move(objects), split);
        CHECK(bvh.primitive_count() == originals.size());

        // Walk the tree from the root (not just the node array) so a leaf
        // that is unreachable would also be caught.
        std::vector<int> times_seen(bvh.primitive_count(), 0);
        std::vector<std::uint32_t> pending{0};
        while (!pending.empty()) {
            const Bvh::Node& node = bvh.nodes()[pending.back()];
            pending.pop_back();
            if (node.is_leaf()) {
                for (std::uint32_t i = node.left_or_first; i < node.left_or_first + node.count; ++i) {
                    ++times_seen[i];
                }
            } else {
                pending.push_back(node.left_or_first);
                pending.push_back(node.left_or_first + 1);
            }
        }

        int wrong = 0;
        std::unordered_set<const Hittable*> stored;
        for (std::size_t i = 0; i < bvh.primitive_count(); ++i) {
            wrong += (times_seen[i] == 1) ? 0 : 1;
            stored.insert(&bvh.primitive(i));
        }
        CHECK(wrong == 0);
        CHECK(stored == originals);  // Same shapes moved in, none copied or lost.
    }
}

//---------------------------------------------------------------- 8. Degenerate split
RT_TEST(bvh_identical_centroids_terminate) {
    // 100 spheres at one point. No axis can separate them, so the build
    // must fall back to splitting by position instead of looping forever.
    constexpr int kCount = 100;
    SceneSpec scene;
    const auto shared = std::make_shared<Lambertian>(Color(0.5, 0.5, 0.5));
    for (int i = 0; i < kCount; ++i) {
        scene.spheres.push_back(SphereSpec{Point3(1.0, 2.0, 3.0), 0.5});
        scene.materials.push_back(shared);
    }
    for (const BvhSplit split : {BvhSplit::Sah, BvhSplit::Median}) {
        const auto list = make_list(scene);
        const Bvh bvh(make_objects(scene), split);
        CHECK(bvh.primitive_count() == static_cast<std::size_t>(kCount));
        CHECK(bvh.depth() < Bvh::kTraversalStackSize);
        for (const Bvh::Node& node : bvh.nodes()) {
            CHECK(node.count <= Bvh::kMaxLeafSize);
        }

        int mismatches = 0;
        for (const TestRay& test : random_rays(kDegenerateRayCount, kRaySeed)) {
            bool was_hit = false;
            mismatches += same_answer(*list, bvh, test.ray, test.t_max, was_hit) ? 0 : 1;
        }
        CHECK(mismatches == 0);
    }
}

//---------------------------------------------------------------- Other edge cases
RT_TEST(aabb_surface_area_of_flat_box_is_positive) {
    // The ground plane's box is almost zero thick on Y. Its area must still
    // be positive so the SAH never divides by zero.
    const Aabb flat{Point3(-20.0, 0.0, -20.0), Point3(20.0, 0.0, 20.0)};
    CHECK_NEAR(flat.surface_area(), 2.0 * 40.0 * 40.0, 1e-9);
    const Aabb cube{Point3(0.0, 0.0, 0.0), Point3(1.0, 2.0, 3.0)};
    CHECK_NEAR(cube.surface_area(), 22.0, 1e-12);
}

RT_TEST(bvh_handles_zero_area_parent_boxes) {
    // Points on a line: every box, and every parent box, has zero area.
    // The SAH would divide by zero, so the build must fall back to the median.
    constexpr int kCount = 64;
    std::vector<std::unique_ptr<Hittable>> objects;
    for (int i = 0; i < kCount; ++i) {
        const Point3 point(static_cast<double>(i), 0.0, 0.0);
        objects.push_back(std::make_unique<BoxOnly>(Aabb{point, point}));
    }
    const Bvh bvh(std::move(objects));
    CHECK(bvh.primitive_count() == static_cast<std::size_t>(kCount));
    CHECK(bvh.depth() < Bvh::kTraversalStackSize);
    CHECK_NEAR(bvh.bounding_box().max.x(), kCount - 1.0, 0.0);
}

RT_TEST(bvh_rejects_non_finite_boxes) {
    std::vector<std::unique_ptr<Hittable>> objects;
    objects.push_back(std::make_unique<BoxOnly>(Aabb{Point3(0.0, 0.0, 0.0), Point3(kInfinity, 1.0, 1.0)}));
    CHECK_THROWS_AS(Bvh(std::move(objects)), std::invalid_argument);
}

RT_TEST(bvh_depth_guard_on_uneven_scene) {
    // Spheres at x = 4^k. The SAH keeps peeling the farthest one or two
    // spheres off the rest, which makes a very deep, list-like tree (with
    // the SAH alone this scene needs more than 64 levels and the build
    // throws). The build must switch to median splits before the tree
    // outgrows the traversal stack, and the answers must still match the
    // list. 250 spheres keeps 4^k (up to 2^498) far from double overflow.
    constexpr int kCount = 250;
    constexpr double kRadius = 0.5;
    SceneSpec scene;
    const auto shared = std::make_shared<Lambertian>(Color(0.5, 0.5, 0.5));
    for (int i = 0; i < kCount; ++i) {
        scene.spheres.push_back(SphereSpec{Point3(std::ldexp(1.0, 2 * i), 0.0, 0.0), kRadius});
        scene.materials.push_back(shared);
    }
    const auto list = make_list(scene);
    const Bvh bvh(make_objects(scene));
    CHECK(bvh.depth() < Bvh::kTraversalStackSize);

    // A ray straight down onto each sphere, plus one along the x axis.
    int mismatches = 0;
    bool was_hit = false;
    for (const SphereSpec& sphere : scene.spheres) {
        const Ray down(sphere.center + Vec3(0.0, 5.0, 0.0), Vec3(0.0, -1.0, 0.0));
        mismatches += same_answer(*list, bvh, down, kInfinity, was_hit) ? 0 : 1;
        CHECK(was_hit);
    }
    const Ray along(Point3(-10.0, 0.0, 0.0), Vec3(1.0, 0.0, 0.0));
    mismatches += same_answer(*list, bvh, along, kInfinity, was_hit) ? 0 : 1;
    CHECK(mismatches == 0);
}

//---------------------------------------------------------------- 9. End to end
RT_TEST(bvh_scene_images_match_list_exactly) {
    RenderSettings settings;
    settings.width = 40;
    settings.height = 32;
    settings.samples_per_pixel = 4;
    settings.max_depth = 8;
    settings.threads = 2;
    const Renderer renderer(settings);
    const Camera camera(scene::original_camera(1.25));

    for (const bool showcase : {false, true}) {
        auto list = showcase ? scene::build_showcase_scene() : scene::build_desk_scene();
        const Image from_list = renderer.render(*list, camera);

        auto source = showcase ? scene::build_showcase_scene() : scene::build_desk_scene();
        const Bvh bvh(source->release_objects());
        CHECK(source->empty());
        const Image from_bvh = renderer.render(bvh, camera);

        int different = 0;
        for (int y = 0; y < settings.height; ++y) {
            for (int x = 0; x < settings.width; ++x) {
                const Color& a = from_list.pixel(x, y);
                const Color& b = from_bvh.pixel(x, y);
                different += (a.x() == b.x() && a.y() == b.y() && a.z() == b.z()) ? 0 : 1;
            }
        }
        CHECK(different == 0);
    }
}

//---------------------------------------------------------------- 10. Depth
RT_TEST(bvh_depth_fits_stack_at_ten_thousand_objects) {
    for (const BvhSplit split : {BvhSplit::Sah, BvhSplit::Median}) {
        const Bvh bvh(make_objects(random_scene(kLargeSphereCount, kSceneSeed)), split);
        CHECK(bvh.primitive_count() == static_cast<std::size_t>(kLargeSphereCount));
        CHECK(bvh.depth() < Bvh::kTraversalStackSize);
        CHECK(bvh.depth() > 0);
    }
}

//---------------------------------------------------------------- Ownership
RT_TEST(hittable_list_release_moves_objects) {
    HittableList list;
    auto first = std::make_unique<Sphere>(Point3(), 1.0, std::make_shared<Lambertian>(Color()));
    const Hittable* first_address = first.get();
    list.add(std::move(first));
    list.add(std::make_unique<Sphere>(Point3(3.0, 0.0, 0.0), 1.0, std::make_shared<Lambertian>(Color())));

    const auto released = list.release_objects();
    CHECK(released.size() == 2);
    CHECK(released.front().get() == first_address);  // Moved, not copied. Order kept.
    CHECK(list.empty());
    CHECK(list.release_objects().empty());
}

//---------------------------------------------------------------- --accel option
RT_TEST(options_accel_defaults_to_bvh) {
    CHECK(app::parse_options({}).accel == app::AccelChoice::Bvh);
}

RT_TEST(options_accel_parses_list_and_bvh) {
    CHECK(app::parse_options({"--accel", "list"}).accel == app::AccelChoice::List);
    CHECK(app::parse_options({"--accel", "bvh"}).accel == app::AccelChoice::Bvh);
    // Last value wins, the same as every other flag.
    CHECK(app::parse_options({"--accel", "list", "--accel", "bvh"}).accel == app::AccelChoice::Bvh);
}

RT_TEST(options_accel_rejects_bad_values) {
    using Args = std::vector<std::string>;
    CHECK_THROWS_AS(app::parse_options(Args{"--accel"}), std::invalid_argument);
    CHECK_THROWS_AS(app::parse_options(Args{"--accel", "kdtree"}), std::invalid_argument);
    CHECK_THROWS_AS(app::parse_options(Args{"--accel", "BVH"}), std::invalid_argument);  // Case matters.
    CHECK_THROWS_AS(app::parse_options(Args{"--accel", ""}), std::invalid_argument);
    CHECK_THROWS_AS(app::parse_options(Args{"--accel", "bvh "}), std::invalid_argument);
    CHECK_THROWS_AS(app::parse_options(Args{"--accel", "--scene"}), std::invalid_argument);
}

RT_TEST(usage_text_documents_accel) {
    CHECK(app::usage_text().find("--accel list|bvh") != std::string::npos);
}
