//=============================================================================
// File:     random.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Seeded random number source for sampling and scattering.
//
// Design notes:
//   The generator is passed in explicitly instead of hidden in a global.
//   That keeps renders reproducible (same seed, same image) and lets
//   each worker thread own its own generator with no locking.
//=============================================================================
#pragma once

#include <cstdint>
#include <random>

#include "core/vec3.h"

namespace rt {

class Rng {
public:
    explicit Rng(std::uint64_t seed) : engine_(seed) {}

    // Uniform value in [0, 1).
    double next() { return distribution_(engine_); }

    // Uniform value in [low, high).
    double next(double low, double high) { return low + (high - low) * next(); }

    // Uniformly distributed direction on the unit sphere.
    // Rejection sampling: draw from the cube until the point is inside the
    // unit sphere, then normalize. Points too close to the center are
    // rejected so normalization never divides by (almost) zero.
    Vec3 unit_vector() {
        constexpr double kMinLengthSquared = 1e-12;
        while (true) {
            const Vec3 candidate(next(-1.0, 1.0), next(-1.0, 1.0), next(-1.0, 1.0));
            const double length_squared = candidate.length_squared();
            if (length_squared > kMinLengthSquared && length_squared <= 1.0) {
                return candidate / std::sqrt(length_squared);
            }
        }
    }

private:
    std::mt19937_64 engine_;
    std::uniform_real_distribution<double> distribution_{0.0, 1.0};
};

}  // namespace rt
