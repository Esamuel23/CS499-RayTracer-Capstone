//=============================================================================
// File:     options.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Command-line options for the renderer.
//
// Design notes:
//   Parsing is kept out of main() so it can be unit tested. Every value is
//   checked before use. Bad input produces a clear message, never a crash
//   or a silent default.
//=============================================================================
#pragma once

#include <string>
#include <vector>

#include "render/renderer.h"

namespace rt::app {

enum class SceneChoice { Desk, Showcase };
enum class CameraChoice { Original, Closeup };

// How the scene is searched for the closest hit. List tests every shape
// (Enhancement One). Bvh uses the bounding volume hierarchy (Enhancement
// Two). Both give the same image; Bvh is faster on large scenes.
enum class AccelChoice { List, Bvh };

struct Options {
    SceneChoice scene = SceneChoice::Showcase;
    CameraChoice camera = CameraChoice::Original;
    AccelChoice accel = AccelChoice::Bvh;
    RenderSettings render;
    std::string output_path = "render.bmp";
    bool show_help = false;
};

// Parses arguments (without the program name). Throws std::invalid_argument
// with a readable message on unknown flags, missing values, non-numeric
// numbers, or values out of range.
Options parse_options(const std::vector<std::string>& arguments);

// Usage text printed by --help.
std::string usage_text();

}  // namespace rt::app
