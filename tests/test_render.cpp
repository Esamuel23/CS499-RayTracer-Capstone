//=============================================================================
// File:     test_render.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Tests for materials, camera, image output, renderer, scene
//           construction, and command-line validation.
//=============================================================================
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "app/options.h"
#include "core/constants.h"
#include "geometry/hittable_list.h"
#include "geometry/sphere.h"
#include "materials/material.h"
#include "render/camera.h"
#include "render/image.h"
#include "render/renderer.h"
#include "scene/desk_scene.h"
#include "test_framework.h"

using namespace rt;

namespace {
constexpr double kTolerance = 1e-9;

HitRecord upward_hit() {
    HitRecord record;
    record.point = Point3(0.0, 0.0, 0.0);
    record.normal = Vec3(0.0, 1.0, 0.0);
    record.front_face = true;
    return record;
}
}  // namespace

//---------------------------------------------------------------- Materials
RT_TEST(lambertian_scatters_above_surface) {
    const Lambertian matte(Color(0.5, 0.25, 0.1));
    Rng rng(7);
    const HitRecord record = upward_hit();
    for (int i = 0; i < 200; ++i) {
        Color attenuation;
        Ray scattered;
        CHECK(matte.scatter(Ray(Point3(0, 1, 0), Vec3(0, -1, 0)), record, rng, attenuation, scattered));
        CHECK(scattered.direction().y() >= 0.0);
        CHECK_NEAR(attenuation.y(), 0.25, kTolerance);
    }
}

RT_TEST(metal_mirror_reflects_exactly) {
    const Metal mirror(Color(1.0, 1.0, 1.0), 0.0);
    Rng rng(7);
    Color attenuation;
    Ray scattered;
    CHECK(mirror.scatter(Ray(Point3(-1, 1, 0), Vec3(1, -1, 0)), upward_hit(), rng, attenuation, scattered));
    const Vec3 direction = unit_vector(scattered.direction());
    CHECK_NEAR(direction.x(), std::sqrt(0.5), 1e-9);
    CHECK_NEAR(direction.y(), std::sqrt(0.5), 1e-9);
}

RT_TEST(metal_fuzz_is_clamped) {
    // Fuzz of 5 would scatter rays anywhere. It must be clamped to 1,
    // so a straight-down ray never reflects below the surface on average.
    const Metal brushed(Color(1.0, 1.0, 1.0), 5.0);
    Rng rng(3);
    int absorbed = 0;
    for (int i = 0; i < 500; ++i) {
        Color attenuation;
        Ray scattered;
        if (!brushed.scatter(Ray(Point3(0, 1, 0), Vec3(0, -1, 0)), upward_hit(), rng, attenuation, scattered)) {
            ++absorbed;
        }
    }
    CHECK(absorbed == 0);  // Straight up plus at most length 1 of fuzz stays above.
}

RT_TEST(dielectric_total_internal_reflection) {
    // Leaving glass at a grazing angle cannot refract. It must reflect.
    const Dielectric glass(1.5);
    Rng rng(11);
    HitRecord record = upward_hit();
    record.front_face = false;  // Ray is inside the glass.
    Color attenuation;
    Ray scattered;
    CHECK(glass.scatter(Ray(Point3(-1, 0.1, 0), Vec3(1.0, -0.1, 0.0)), record, rng, attenuation, scattered));
    CHECK(scattered.direction().y() > 0.0);  // Reflected back up, not through.
    CHECK_NEAR(attenuation.x(), 1.0, kTolerance);
}

RT_TEST(dielectric_rejects_invalid_index) {
    CHECK_THROWS_AS(Dielectric(0.0), std::invalid_argument);
}

//---------------------------------------------------------------- Camera
RT_TEST(camera_center_ray_matches_original_front) {
    const Camera camera(scene::original_camera(1.25));
    const Ray center = camera.generate_ray(0.5, 0.5);
    const Vec3 expected = unit_vector(scene::kCameraFront);
    const Vec3 actual = unit_vector(center.direction());
    CHECK_NEAR(actual.x(), expected.x(), 1e-9);
    CHECK_NEAR(actual.y(), expected.y(), 1e-9);
    CHECK_NEAR(actual.z(), expected.z(), 1e-9);
    CHECK_NEAR(center.origin().y(), 5.0, kTolerance);
}

RT_TEST(camera_top_of_image_looks_higher) {
    const Camera camera(scene::original_camera(1.25));
    CHECK(camera.generate_ray(0.5, 1.0).direction().y() > camera.generate_ray(0.5, 0.0).direction().y());
    CHECK(camera.generate_ray(1.0, 0.5).direction().x() > camera.generate_ray(0.0, 0.5).direction().x());
}

RT_TEST(camera_rejects_invalid_settings) {
    CameraSettings settings = scene::original_camera(1.25);
    settings.front = Vec3(0.0, 0.0, 0.0);
    CHECK_THROWS_AS(Camera{settings}, std::invalid_argument);

    settings = scene::original_camera(1.25);
    settings.up = Vec3(0.0, -0.5, -2.0);  // Parallel to front.
    CHECK_THROWS_AS(Camera{settings}, std::invalid_argument);

    settings = scene::original_camera(1.25);
    settings.vertical_fov_degrees = 180.0;
    CHECK_THROWS_AS(Camera{settings}, std::invalid_argument);

    CHECK_THROWS_AS(Camera{scene::original_camera(0.0)}, std::invalid_argument);
}

//---------------------------------------------------------------- Image
RT_TEST(image_bounds_are_checked) {
    Image image(4, 3);
    CHECK_THROWS_AS(image.set_pixel(4, 0, Color()), std::out_of_range);
    CHECK_THROWS_AS(image.pixel(0, -1), std::out_of_range);
    CHECK_THROWS_AS(Image(0, 10), std::invalid_argument);
}

RT_TEST(image_to_byte_clamps_and_gamma_corrects) {
    CHECK(Image::to_byte(-1.0) == 0);
    CHECK(Image::to_byte(2.0) == 255);
    CHECK(Image::to_byte(0.25) == 128);  // sqrt(0.25) = 0.5, 0.5 * 255 rounds to 128.
    CHECK(Image::to_byte(std::nan("")) == 0);
}

RT_TEST(image_writes_valid_bmp) {
    Image image(3, 2);  // Width 3 forces row padding (9 bytes to 12).
    image.set_pixel(0, 0, Color(1.0, 0.0, 0.0));
    const std::string path = "test_output.bmp";
    image.write(path);

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    CHECK(file.good());
    const std::streamoff size = file.tellg();
    CHECK(size == 54 + 12 * 2);

    file.seekg(0);
    char signature[2] = {0, 0};
    file.read(signature, 2);
    CHECK(signature[0] == 'B' && signature[1] == 'M');
    file.close();
    std::remove(path.c_str());
}

RT_TEST(image_rejects_unknown_extension_and_bad_path) {
    const Image image(2, 2);
    CHECK_THROWS_AS(image.write("render.png"), std::invalid_argument);
    CHECK_THROWS_AS(image.write("no_such_folder_499/render.bmp"), std::runtime_error);
}

//---------------------------------------------------------------- Renderer
RT_TEST(renderer_empty_scene_shows_sky) {
    const HittableList empty;
    Rng rng(1);
    const Color straight_up = Renderer::trace(Ray(Point3(), Vec3(0, 1, 0)), empty, 5, rng);
    CHECK_NEAR(straight_up.x(), 0.5, kTolerance);
    CHECK_NEAR(straight_up.z(), 1.0, kTolerance);
}

RT_TEST(renderer_depth_zero_returns_black) {
    const HittableList empty;
    Rng rng(1);
    const Color color = Renderer::trace(Ray(Point3(), Vec3(0, 1, 0)), empty, 0, rng);
    CHECK_NEAR(color.length(), 0.0, kTolerance);
}

RT_TEST(renderer_is_deterministic_across_thread_counts) {
    const auto world = scene::build_showcase_scene();
    const Camera camera(scene::original_camera(1.25));

    RenderSettings settings;
    settings.width = 20;
    settings.height = 16;
    settings.samples_per_pixel = 4;
    settings.max_depth = 8;

    settings.threads = 1;
    const Image single = Renderer(settings).render(*world, camera);
    settings.threads = 4;
    const Image multi = Renderer(settings).render(*world, camera);

    for (int y = 0; y < settings.height; ++y) {
        for (int x = 0; x < settings.width; ++x) {
            CHECK_NEAR(single.pixel(x, y).x(), multi.pixel(x, y).x(), 0.0);
        }
    }
}

RT_TEST(render_settings_reject_out_of_range) {
    RenderSettings settings;
    settings.width = 0;
    CHECK_THROWS_AS(settings.validate(), std::invalid_argument);
    settings = RenderSettings{};
    settings.samples_per_pixel = 20000;
    CHECK_THROWS_AS(settings.validate(), std::invalid_argument);
    settings = RenderSettings{};
    settings.threads = -1;
    CHECK_THROWS_AS(settings.validate(), std::invalid_argument);
}

//---------------------------------------------------------------- Scene
RT_TEST(desk_legs_reproduce_original_positions) {
    // Original literals from SceneManager::RenderScene.
    const Point3 expected[scene::kLegCount] = {
        Point3(-2.7, 1.5, -1.2), Point3(2.7, 1.5, -1.2),
        Point3(-2.7, 1.5, 1.2), Point3(2.7, 1.5, 1.2)};
    std::size_t i = 0;
    for (const Point3& leg : scene::leg_positions()) {
        CHECK_NEAR(leg.x(), expected[i].x(), 1e-12);
        CHECK_NEAR(leg.y(), expected[i].y(), 1e-12);
        CHECK_NEAR(leg.z(), expected[i].z(), 1e-12);
        ++i;
    }
}

RT_TEST(desk_scene_object_counts) {
    CHECK(scene::build_desk_scene()->size() == 6);      // Ground, top, 4 legs.
    CHECK(scene::build_showcase_scene()->size() == 10); // Plus 4 props.
}

RT_TEST(camera_center_ray_hits_scene) {
    const auto world = scene::build_desk_scene();
    const Camera camera(scene::original_camera(1.25));
    HitRecord record;
    CHECK(world->hit(camera.generate_ray(0.5, 0.5), kHitEpsilon, kInfinity, record));
}

//---------------------------------------------------------------- Options
RT_TEST(options_parse_valid_arguments) {
    const app::Options options = app::parse_options(
        {"--scene", "desk", "--camera", "closeup", "--width", "320", "--height", "256", "--samples", "8", "--out", "a.ppm"});
    CHECK(options.scene == app::SceneChoice::Desk);
    CHECK(options.camera == app::CameraChoice::Closeup);
    CHECK(options.render.width == 320);
    CHECK(options.render.samples_per_pixel == 8);
    CHECK(options.output_path == "a.ppm");
}

RT_TEST(options_reject_bad_input) {
    using Args = std::vector<std::string>;
    CHECK_THROWS_AS(app::parse_options(Args{"--width"}), std::invalid_argument);
    CHECK_THROWS_AS(app::parse_options(Args{"--width", "12abc"}), std::invalid_argument);
    CHECK_THROWS_AS(app::parse_options(Args{"--width", "-5"}), std::invalid_argument);
    CHECK_THROWS_AS(app::parse_options(Args{"--samples", "99999999999"}), std::invalid_argument);
    CHECK_THROWS_AS(app::parse_options(Args{"--scene", "kitchen"}), std::invalid_argument);
    CHECK_THROWS_AS(app::parse_options(Args{"--camera", "fisheye"}), std::invalid_argument);
    CHECK_THROWS_AS(app::parse_options(Args{"--bogus", "1"}), std::invalid_argument);
}
