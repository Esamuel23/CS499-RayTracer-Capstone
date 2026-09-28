//=============================================================================
// File:     constants.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Named numeric constants shared across the renderer.
//
// Design notes:
//   The code review found magic numbers throughout the original scene code.
//   Every tolerance here has a name and a reason.
//=============================================================================
#pragma once

#include <limits>

namespace rt {

constexpr double kPi = 3.14159265358979323846;
constexpr double kInfinity = std::numeric_limits<double>::infinity();

// Minimum ray parameter accepted as a hit. A bounced ray starts exactly on
// a surface. Rounding error can make it hit that same surface at t close to
// zero ("shadow acne"). Ignoring hits below this value prevents that.
constexpr double kHitEpsilon = 1e-4;

// Below this magnitude a direction component is treated as zero.
// Used to detect rays parallel to a slab, plane, or axis instead of
// dividing by (nearly) zero.
constexpr double kParallelEpsilon = 1e-12;

constexpr double degrees_to_radians(double degrees) { return degrees * kPi / 180.0; }

}  // namespace rt
