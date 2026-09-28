//=============================================================================
// File:     renderer.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Multi-sample, multi-threaded ray tracing of a scene.
//=============================================================================
#include "render/renderer.h"

#include <algorithm>
#include <atomic>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include "core/constants.h"
#include "materials/material.h"

namespace rt {
namespace {

// Upper limits that keep a typo on the command line from asking for
// gigabytes of memory or hours of rendering.
constexpr int kMaxDimension = 8192;
constexpr int kMaxSamples = 10000;
constexpr int kMaxDepth = 500;
constexpr int kMaxThreads = 256;

// Spreads row numbers across the seed space so each row gets an
// independent random stream. Rows are seeded by index, not by thread,
// so the image is identical no matter how many threads run.
constexpr std::uint64_t kRowSeedMultiplier = 0x9E3779B97F4A7C15ull;

}  // namespace

void RenderSettings::validate() const {
    if (width < 1 || width > kMaxDimension || height < 1 || height > kMaxDimension) {
        throw std::invalid_argument("Width and height must be between 1 and 8192");
    }
    if (samples_per_pixel < 1 || samples_per_pixel > kMaxSamples) {
        throw std::invalid_argument("Samples per pixel must be between 1 and 10000");
    }
    if (max_depth < 1 || max_depth > kMaxDepth) {
        throw std::invalid_argument("Max depth must be between 1 and 500");
    }
    if (threads < 0 || threads > kMaxThreads) {
        throw std::invalid_argument("Threads must be between 0 and 256");
    }
}

Renderer::Renderer(const RenderSettings& settings) : settings_(settings) {
    settings_.validate();
}

Color Renderer::background(const Ray& ray) {
    // Blend white at the horizon to light blue overhead.
    const Vec3 direction = unit_vector(ray.direction());
    const double blend = 0.5 * (direction.y() + 1.0);
    return (1.0 - blend) * Color(1.0, 1.0, 1.0) + blend * Color(0.5, 0.7, 1.0);
}

Color Renderer::trace(const Ray& ray, const Hittable& world, int depth, Rng& rng) {
    // Bounce limit reached. No more light is gathered.
    if (depth <= 0) {
        return Color(0.0, 0.0, 0.0);
    }

    HitRecord record;
    if (!world.hit(ray, kHitEpsilon, kInfinity, record)) {
        return background(ray);
    }

    // Every shape sets a material. Guard anyway so a bad shape shows up
    // as black instead of crashing the render.
    if (record.material == nullptr) {
        return Color(0.0, 0.0, 0.0);
    }

    Color attenuation;
    Ray scattered;
    if (record.material->scatter(ray, record, rng, attenuation, scattered)) {
        return attenuation * trace(scattered, world, depth - 1, rng);
    }
    return Color(0.0, 0.0, 0.0);  // Absorbed.
}

void Renderer::render_row(int y, const Hittable& world, const Camera& camera, Image& image) const {
    Rng rng(settings_.seed + kRowSeedMultiplier * static_cast<std::uint64_t>(y + 1));

    // Image row 0 is the top. Camera t = 1 is the top. Flip once here.
    const double row_from_bottom = static_cast<double>(settings_.height - 1 - y);
    const double inverse_samples = 1.0 / settings_.samples_per_pixel;

    for (int x = 0; x < settings_.width; ++x) {
        Color total(0.0, 0.0, 0.0);
        for (int sample = 0; sample < settings_.samples_per_pixel; ++sample) {
            // Jitter inside the pixel so edges are averaged (anti-aliasing).
            const double s = (x + rng.next()) / settings_.width;
            const double t = (row_from_bottom + rng.next()) / settings_.height;
            total += trace(camera.generate_ray(s, t), world, settings_.max_depth, rng);
        }
        image.set_pixel(x, y, total * inverse_samples);
    }
}

Image Renderer::render(const Hittable& world, const Camera& camera) const {
    Image image(settings_.width, settings_.height);

    unsigned requested = static_cast<unsigned>(settings_.threads);
    if (requested == 0) {
        requested = std::max(1u, std::thread::hardware_concurrency());
    }
    const unsigned thread_count =
        std::min(requested, static_cast<unsigned>(settings_.height));

    // Workers pull the next unrendered row. Each row is written by exactly
    // one thread, so pixel writes need no lock.
    std::atomic<int> next_row{0};
    std::exception_ptr first_error;
    std::mutex error_mutex;

    auto worker = [&]() {
        try {
            for (int y = next_row++; y < settings_.height; y = next_row++) {
                render_row(y, world, camera, image);
            }
        } catch (...) {
            // Keep the first failure and rethrow it on the calling thread.
            // An exception escaping a std::thread would end the program.
            const std::lock_guard<std::mutex> lock(error_mutex);
            if (!first_error) {
                first_error = std::current_exception();
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    for (unsigned i = 0; i < thread_count; ++i) {
        workers.emplace_back(worker);
    }
    for (std::thread& thread : workers) {
        thread.join();
    }

    if (first_error) {
        std::rethrow_exception(first_error);
    }
    return image;
}

}  // namespace rt
