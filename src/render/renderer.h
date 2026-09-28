//=============================================================================
// File:     renderer.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Owns the sampling loop and the recursive trace.
//
// Design notes:
//   The original render loop lived in MainCode.cpp and mixed window events,
//   OpenGL state, and drawing. Here rendering is one class with one input
//   (a scene and a camera) and one output (an Image). It depends only on
//   the Hittable and Material interfaces, never on a concrete shape.
//=============================================================================
#pragma once

#include <cstdint>

#include "core/hittable.h"
#include "core/random.h"
#include "render/camera.h"
#include "render/image.h"

namespace rt {

struct RenderSettings {
    int width = 800;
    int height = 640;
    int samples_per_pixel = 64;  // Rays averaged per pixel (anti-aliasing and noise).
    int max_depth = 20;          // Bounce limit. Stops infinite mirror recursion.
    std::uint64_t seed = 499;    // Same seed, same image.
    int threads = 0;             // 0 means use every hardware thread.

    // Throws std::invalid_argument when any value is out of range.
    void validate() const;
};

class Renderer {
public:
    // Validates the settings. Throws std::invalid_argument if they are bad.
    explicit Renderer(const RenderSettings& settings);

    Image render(const Hittable& world, const Camera& camera) const;

    // Color seen along one ray. Public so it can be unit tested.
    static Color trace(const Ray& ray, const Hittable& world, int depth, Rng& rng);

    // Sky gradient used when a ray escapes the scene. This is the only
    // light source, so every shadow comes from blocked sky.
    static Color background(const Ray& ray);

private:
    void render_row(int y, const Hittable& world, const Camera& camera, Image& image) const;

    RenderSettings settings_;
};

}  // namespace rt
