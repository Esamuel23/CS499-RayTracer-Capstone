//=============================================================================
// File:     bench_accel.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     October 2026
// Purpose:  Benchmark: HittableList versus Bvh on growing random scenes.
//
// Design notes:
//   This is the evidence for the Enhancement Two trade-off: how build time
//   and render time grow with the number of shapes for each structure.
//
//   Scene: N spheres of one small radius, centers uniform in a cube, fixed
//   seed. N = 1, 2, 5, 10, 100, 1,000, 10,000. Render: fixed resolution, 1 sample per
//   pixel, max depth 1 (primary rays only, so the time is dominated by the
//   closest-hit search), 1 worker thread. Each case runs kDefaultRuns times
//   and the median is reported, which ignores one-off slow runs.
//
//   Structures: list (O(n) per ray), bvh_sah (default build) and bvh_median
//   (the simple build), so the SAH can be judged against the plain split.
//
//   The same source builds two programs. raytracer_bench links the normal
//   library and reports times. raytracer_bench_stats links a copy of the
//   library with RT_ENABLE_TRAVERSAL_STATS and reports box and shape tests
//   per ray. Its times are slower and are not used.
//
//   Every BVH image is also compared pixel for pixel with the list image, so
//   the speedup is never measured on a wrong answer.
//
//   Usage: raytracer_bench [--runs N] [--out FILE.csv]
//=============================================================================
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "accel/bvh.h"
#include "accel/traversal_stats.h"
#include "core/constants.h"
#include "core/random.h"
#include "geometry/hittable_list.h"
#include "geometry/sphere.h"
#include "materials/material.h"
#include "render/camera.h"
#include "render/image.h"
#include "render/renderer.h"

namespace {

using Clock = std::chrono::steady_clock;

constexpr std::uint64_t kSceneSeed = 499;
// The spec's sizes (10 to 10,000) plus 1, 2, and 5, to find where the
// list stops being faster.
constexpr int kSceneSizes[] = {1, 2, 5, 10, 100, 1000, 10000};
constexpr double kCubeHalfExtent = 5.0;  // Centers uniform in [-5, 5]^3.
constexpr double kSphereRadius = 0.2;
constexpr rt::Color kSphereColor{0.6, 0.6, 0.6};

constexpr int kImageWidth = 400;
constexpr int kImageHeight = 300;
constexpr int kDefaultRuns = 5;
constexpr int kMaxRuns = 101;

// Camera on the +Z axis looking at the cube, far enough to frame all of it.
constexpr rt::Point3 kCameraPosition{0.0, 0.0, 15.0};
constexpr rt::Vec3 kCameraFront{0.0, 0.0, -1.0};
constexpr rt::Vec3 kCameraUp{0.0, 1.0, 0.0};
constexpr double kCameraFovDegrees = 45.0;

enum class Structure { List, BvhSah, BvhMedian };
constexpr Structure kStructures[] = {Structure::List, Structure::BvhSah, Structure::BvhMedian};

const char* name_of(Structure structure) {
    switch (structure) {
        case Structure::List:
            return "list";
        case Structure::BvhSah:
            return "bvh_sah";
        case Structure::BvhMedian:
            return "bvh_median";
    }
    return "unknown";
}

struct BenchOptions {
    int runs = kDefaultRuns;
    std::string output_path;
};

// Whole-string integer parse, same rule as the main program's options.
int to_int(const std::string& flag, const std::string& text) {
    std::size_t consumed = 0;
    int value = 0;
    try {
        value = std::stoi(text, &consumed);
    } catch (const std::exception&) {
        throw std::invalid_argument(flag + " expects a whole number, got \"" + text + "\"");
    }
    if (consumed != text.size()) {
        throw std::invalid_argument(flag + " expects a whole number, got \"" + text + "\"");
    }
    return value;
}

BenchOptions parse_arguments(const std::vector<std::string>& arguments) {
    BenchOptions options;
    options.output_path = rt::kTraversalStatsEnabled ? "bvh_counts.csv" : "bvh_timing.csv";
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        const std::string& flag = arguments[i];
        if (i + 1 >= arguments.size()) {
            throw std::invalid_argument("Missing value after " + flag);
        }
        const std::string& value = arguments[++i];
        if (flag == "--runs") {
            options.runs = to_int(flag, value);
            if (options.runs < 1 || options.runs > kMaxRuns) {
                throw std::invalid_argument("--runs must be between 1 and 101");
            }
        } else if (flag == "--out") {
            options.output_path = value;
        } else {
            throw std::invalid_argument("Unknown option: " + flag);
        }
    }
    return options;
}

// The scene as plain shapes, rebuilt for every run because building a BVH
// takes ownership of them. Same seed, so every run gets the same scene.
std::vector<std::unique_ptr<rt::Hittable>> make_spheres(int count,
                                                        const std::shared_ptr<const rt::Material>& material) {
    rt::Rng rng(kSceneSeed);
    std::vector<std::unique_ptr<rt::Hittable>> spheres;
    spheres.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const rt::Point3 center(rng.next(-kCubeHalfExtent, kCubeHalfExtent),
                                rng.next(-kCubeHalfExtent, kCubeHalfExtent),
                                rng.next(-kCubeHalfExtent, kCubeHalfExtent));
        spheres.push_back(std::make_unique<rt::Sphere>(center, kSphereRadius, material));
    }
    return spheres;
}

std::unique_ptr<rt::Hittable> build(Structure structure,
                                    std::vector<std::unique_ptr<rt::Hittable>> shapes) {
    if (structure == Structure::List) {
        auto list = std::make_unique<rt::HittableList>();
        for (auto& shape : shapes) {
            list->add(std::move(shape));
        }
        return list;
    }
    const rt::BvhSplit split =
        (structure == Structure::BvhSah) ? rt::BvhSplit::Sah : rt::BvhSplit::Median;
    return std::make_unique<rt::Bvh>(std::move(shapes), split);
}

double milliseconds_between(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double median(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    const std::size_t middle = values.size() / 2;
    if (values.size() % 2 == 1) {
        return values[middle];
    }
    return 0.5 * (values[middle - 1] + values[middle]);
}

bool images_equal(const rt::Image& a, const rt::Image& b) {
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            const rt::Color& p = a.pixel(x, y);
            const rt::Color& q = b.pixel(x, y);
            if (!(p.x() == q.x() && p.y() == q.y() && p.z() == q.z())) {
                return false;
            }
        }
    }
    return true;
}

// Fraction of primary rays (pixel centers) that hit any sphere. Context
// for the counts: a ray that misses the root box costs the BVH one test.
double hit_fraction(const rt::Hittable& world, const rt::Camera& camera) {
    int hits = 0;
    for (int y = 0; y < kImageHeight; ++y) {
        for (int x = 0; x < kImageWidth; ++x) {
            const double s = (x + 0.5) / kImageWidth;
            const double t = (y + 0.5) / kImageHeight;
            rt::HitRecord record;
            hits += world.hit(camera.generate_ray(s, t), rt::kHitEpsilon, rt::kInfinity, record) ? 1 : 0;
        }
    }
    return static_cast<double>(hits) / (static_cast<double>(kImageWidth) * kImageHeight);
}

struct Result {
    int count = 0;
    Structure structure = Structure::List;
    double build_ms = 0.0;
    double render_ms = 0.0;
    double box_tests_per_ray = 0.0;
    double primitive_tests_per_ray = 0.0;
    double hit_fraction = 0.0;
    std::size_t nodes = 0;
    int depth = 0;
    bool matches_list = true;
};

void write_csv(const std::string& path, const std::vector<Result>& results, int runs) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open output file: " + path);
    }
    file << std::fixed;
    if constexpr (rt::kTraversalStatsEnabled) {
        file << "n,structure,box_tests_per_ray,primitive_tests_per_ray,hit_fraction\n";
        for (const Result& r : results) {
            file << r.count << ',' << name_of(r.structure) << ',' << std::setprecision(3)
                 << r.box_tests_per_ray << ',' << r.primitive_tests_per_ray << ','
                 << std::setprecision(4) << r.hit_fraction << '\n';
        }
    } else {
        file << "n,structure,build_ms,render_ms,runs,nodes,depth,hit_fraction,matches_list\n";
        for (const Result& r : results) {
            file << r.count << ',' << name_of(r.structure) << ',' << std::setprecision(4)
                 << r.build_ms << ',' << std::setprecision(2) << r.render_ms << ',' << runs << ','
                 << r.nodes << ',' << r.depth << ',' << std::setprecision(4) << r.hit_fraction << ','
                 << (r.matches_list ? "yes" : "no") << '\n';
        }
    }
    if (!file) {
        throw std::runtime_error("Failed while writing output file: " + path);
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const BenchOptions options = parse_arguments(std::vector<std::string>(argv + 1, argv + argc));

        rt::RenderSettings settings;
        settings.width = kImageWidth;
        settings.height = kImageHeight;
        settings.samples_per_pixel = 1;
        settings.max_depth = 1;  // Primary rays only.
        settings.threads = 1;
        const rt::Renderer renderer(settings);
        const rt::Camera camera(rt::CameraSettings{kCameraPosition, kCameraFront, kCameraUp,
                                                   kCameraFovDegrees,
                                                   static_cast<double>(kImageWidth) / kImageHeight});
        const double rays = static_cast<double>(kImageWidth) * kImageHeight;
        const auto material = std::make_shared<rt::Lambertian>(kSphereColor);

        std::cout << (rt::kTraversalStatsEnabled ? "Counting tests per ray" : "Timing") << ": "
                  << kImageWidth << "x" << kImageHeight << ", 1 sample, depth 1, 1 thread, median of "
                  << options.runs << " runs\n";

        std::vector<Result> results;
        for (const int count : kSceneSizes) {
            // Measured once per scene, outside every timed region. A BVH
            // gives the same answers as the list and is much faster here.
            const double scene_hit_fraction =
                hit_fraction(rt::Bvh(make_spheres(count, material)), camera);
            std::unique_ptr<rt::Image> list_image;
            for (const Structure structure : kStructures) {
                Result result;
                result.count = count;
                result.structure = structure;
                result.hit_fraction = scene_hit_fraction;
                std::vector<double> build_times;
                std::vector<double> render_times;

                for (int run = 0; run < options.runs; ++run) {
                    auto shapes = make_spheres(count, material);  // Not timed.

                    const Clock::time_point build_start = Clock::now();
                    const std::unique_ptr<rt::Hittable> world = build(structure, std::move(shapes));
                    const Clock::time_point build_end = Clock::now();

                    rt::reset_traversal_stats();
                    const Clock::time_point render_start = Clock::now();
                    const rt::Image image = renderer.render(*world, camera);
                    const Clock::time_point render_end = Clock::now();
                    const rt::TraversalStats stats = rt::traversal_stats();

                    build_times.push_back(milliseconds_between(build_start, build_end));
                    render_times.push_back(milliseconds_between(render_start, render_end));

                    if (run == 0) {
                        // Counts and image are identical on every run, so
                        // the first run is enough.
                        result.box_tests_per_ray = static_cast<double>(stats.box_tests) / rays;
                        result.primitive_tests_per_ray = static_cast<double>(stats.primitive_tests) / rays;
                        if (const auto* bvh = dynamic_cast<const rt::Bvh*>(world.get())) {
                            result.nodes = bvh->nodes().size();
                            result.depth = bvh->depth();
                        }
                        if (structure == Structure::List) {
                            list_image = std::make_unique<rt::Image>(image);
                        } else {
                            result.matches_list = images_equal(*list_image, image);
                        }
                    }
                }

                result.build_ms = median(build_times);
                result.render_ms = median(render_times);
                results.push_back(result);

                std::cout << std::fixed << std::setw(6) << count << "  " << std::setw(10)
                          << name_of(structure) << "  build " << std::setprecision(3) << std::setw(9)
                          << result.build_ms << " ms  render " << std::setprecision(1) << std::setw(9)
                          << result.render_ms << " ms";
                if constexpr (rt::kTraversalStatsEnabled) {
                    std::cout << "  boxes/ray " << std::setprecision(2) << result.box_tests_per_ray
                              << "  shapes/ray " << result.primitive_tests_per_ray;
                }
                std::cout << (result.matches_list ? "" : "  IMAGE DIFFERS FROM LIST") << "\n";
                if (!result.matches_list) {
                    throw std::runtime_error("BVH image differs from the list image");
                }
            }
        }

        write_csv(options.output_path, results, options.runs);
        std::cout << "Wrote " << options.output_path << "\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n"
                  << "Usage: raytracer_bench [--runs N] [--out FILE.csv]\n";
        return 1;
    }
}
