//=============================================================================
// File:     image.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  In-memory image and writers for BMP and PPM files.
//
// Design notes:
//   The original drew into an OpenGL window. The ray tracer writes an image
//   file, so it needs no window, no GPU, and no graphics library.
//   BMP opens on Windows with no extra software. PPM is the simplest
//   format to inspect by hand.
//=============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/vec3.h"

namespace rt {

class Image {
public:
    // Throws std::invalid_argument if width or height <= 0.
    Image(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }

    // (0, 0) is the top-left pixel. Both throw std::out_of_range when
    // x or y is outside the image.
    void set_pixel(int x, int y, const Color& color);
    const Color& pixel(int x, int y) const;

    // Linear color to an 8-bit channel: clamp, gamma 2.0, scale to 0-255.
    // NaN becomes 0 so one bad sample cannot corrupt the file.
    static std::uint8_t to_byte(double linear);

    // Write by file extension (.bmp or .ppm). Throws std::invalid_argument
    // for any other extension and std::runtime_error if the write fails.
    void write(const std::string& path) const;
    void write_bmp(const std::string& path) const;
    void write_ppm(const std::string& path) const;

private:
    std::size_t index(int x, int y) const;

    int width_;
    int height_;
    std::vector<Color> pixels_;
};

}  // namespace rt
