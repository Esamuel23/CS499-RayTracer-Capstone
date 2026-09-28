//=============================================================================
// File:     options.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Strict parsing and validation of command-line options.
//=============================================================================
#include "app/options.h"

#include <cstddef>
#include <stdexcept>

namespace rt::app {
namespace {

// Reads a whole string as an integer. Rejects partial numbers like "12abc"
// that std::stoi would quietly accept as 12.
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

}  // namespace

Options parse_options(const std::vector<std::string>& arguments) {
    Options options;

    for (std::size_t i = 0; i < arguments.size(); ++i) {
        const std::string& flag = arguments[i];

        if (flag == "--help" || flag == "-h") {
            options.show_help = true;
            continue;
        }

        // Every other flag takes exactly one value.
        if (i + 1 >= arguments.size()) {
            throw std::invalid_argument("Missing value after " + flag);
        }
        const std::string& value = arguments[++i];

        if (flag == "--scene") {
            if (value == "desk") {
                options.scene = SceneChoice::Desk;
            } else if (value == "showcase") {
                options.scene = SceneChoice::Showcase;
            } else {
                throw std::invalid_argument("--scene must be desk or showcase");
            }
        } else if (flag == "--camera") {
            if (value == "original") {
                options.camera = CameraChoice::Original;
            } else if (value == "closeup") {
                options.camera = CameraChoice::Closeup;
            } else {
                throw std::invalid_argument("--camera must be original or closeup");
            }
        } else if (flag == "--width") {
            options.render.width = to_int(flag, value);
        } else if (flag == "--height") {
            options.render.height = to_int(flag, value);
        } else if (flag == "--samples") {
            options.render.samples_per_pixel = to_int(flag, value);
        } else if (flag == "--depth") {
            options.render.max_depth = to_int(flag, value);
        } else if (flag == "--threads") {
            options.render.threads = to_int(flag, value);
        } else if (flag == "--seed") {
            const int seed = to_int(flag, value);
            if (seed < 0) {
                throw std::invalid_argument("--seed must not be negative");
            }
            options.render.seed = static_cast<std::uint64_t>(seed);
        } else if (flag == "--out") {
            options.output_path = value;
        } else {
            throw std::invalid_argument("Unknown option: " + flag);
        }
    }

    // Range checks live in one place, RenderSettings::validate().
    options.render.validate();
    return options;
}

std::string usage_text() {
    return "Usage: raytracer [options]\n"
           "  --scene desk|showcase  Scene to render (default showcase)\n"
           "  --camera original|closeup  Camera view (default original)\n"
           "  --width N              Image width in pixels (default 800)\n"
           "  --height N             Image height in pixels (default 640)\n"
           "  --samples N            Samples per pixel (default 64)\n"
           "  --depth N              Maximum bounces per ray (default 20)\n"
           "  --threads N            Worker threads, 0 = all cores (default 0)\n"
           "  --seed N               Random seed (default 499)\n"
           "  --out FILE             Output .bmp or .ppm (default render.bmp)\n"
           "  --help                 Show this message\n";
}

}  // namespace rt::app
