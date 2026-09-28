//=============================================================================
// File:     image.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Pixel storage, color conversion, and file output.
//=============================================================================
#include "render/image.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace rt {
namespace {

constexpr int kBmpHeaderSize = 54;       // 14-byte file header + 40-byte info header.
constexpr int kBmpBytesPerPixel = 3;     // 24-bit color, stored as B, G, R.
constexpr int kBmpPixelsPerMeter = 2835; // 72 DPI. Metadata only.

// BMP stores every multi-byte field in little-endian order.
void put_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
}

void put_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFu));
    }
}

bool ends_with(const std::string& text, const std::string& suffix) {
    if (suffix.size() > text.size()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), text.rbegin(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) ==
               std::tolower(static_cast<unsigned char>(b));
    });
}

// Opens the file and writes the bytes, or throws with the path in the
// message. Callers never get a silent failure.
void write_bytes(const std::string& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open output file: " + path);
    }
    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!file) {
        throw std::runtime_error("Failed while writing output file: " + path);
    }
}

}  // namespace

Image::Image(int width, int height) : width_(width), height_(height) {
    if (width_ <= 0 || height_ <= 0) {
        throw std::invalid_argument("Image dimensions must be positive");
    }
    pixels_.resize(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_));
}

std::size_t Image::index(int x, int y) const {
    // Every index is checked before use. The code review found the
    // original wrote to a fixed array with no bounds check.
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        throw std::out_of_range("Pixel coordinate outside the image");
    }
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(x);
}

void Image::set_pixel(int x, int y, const Color& color) { pixels_[index(x, y)] = color; }

const Color& Image::pixel(int x, int y) const { return pixels_[index(x, y)]; }

std::uint8_t Image::to_byte(double linear) {
    if (std::isnan(linear)) {
        linear = 0.0;
    }
    const double clamped = std::clamp(linear, 0.0, 1.0);
    const double gamma_corrected = std::sqrt(clamped);  // Gamma 2.0.
    return static_cast<std::uint8_t>(std::lround(gamma_corrected * 255.0));
}

void Image::write(const std::string& path) const {
    if (ends_with(path, ".bmp")) {
        write_bmp(path);
    } else if (ends_with(path, ".ppm")) {
        write_ppm(path);
    } else {
        throw std::invalid_argument("Output file must end in .bmp or .ppm: " + path);
    }
}

void Image::write_bmp(const std::string& path) const {
    // Each row is padded to a multiple of 4 bytes.
    const std::uint32_t row_bytes = static_cast<std::uint32_t>(width_) * kBmpBytesPerPixel;
    const std::uint32_t row_stride = (row_bytes + 3u) & ~3u;
    const std::uint32_t pixel_bytes = row_stride * static_cast<std::uint32_t>(height_);
    const std::uint32_t file_size = kBmpHeaderSize + pixel_bytes;

    std::vector<std::uint8_t> bytes;
    bytes.reserve(file_size);

    // File header.
    bytes.push_back('B');
    bytes.push_back('M');
    put_u32(bytes, file_size);
    put_u32(bytes, 0);  // Reserved.
    put_u32(bytes, kBmpHeaderSize);

    // Info header (BITMAPINFOHEADER).
    put_u32(bytes, 40);
    put_u32(bytes, static_cast<std::uint32_t>(width_));
    put_u32(bytes, static_cast<std::uint32_t>(height_));  // Positive: rows stored bottom-up.
    put_u16(bytes, 1);                                     // Color planes.
    put_u16(bytes, 24);                                    // Bits per pixel.
    put_u32(bytes, 0);                                     // No compression.
    put_u32(bytes, pixel_bytes);
    put_u32(bytes, kBmpPixelsPerMeter);
    put_u32(bytes, kBmpPixelsPerMeter);
    put_u32(bytes, 0);  // Palette colors.
    put_u32(bytes, 0);  // Important colors.

    // Pixel rows, bottom row first.
    for (int y = height_ - 1; y >= 0; --y) {
        for (int x = 0; x < width_; ++x) {
            const Color& color = pixel(x, y);
            bytes.push_back(to_byte(color.z()));
            bytes.push_back(to_byte(color.y()));
            bytes.push_back(to_byte(color.x()));
        }
        for (std::uint32_t pad = row_bytes; pad < row_stride; ++pad) {
            bytes.push_back(0);
        }
    }

    write_bytes(path, bytes);
}

void Image::write_ppm(const std::string& path) const {
    const std::string header =
        "P6\n" + std::to_string(width_) + " " + std::to_string(height_) + "\n255\n";

    std::vector<std::uint8_t> bytes(header.begin(), header.end());
    bytes.reserve(header.size() + pixels_.size() * 3);
    for (const Color& color : pixels_) {
        bytes.push_back(to_byte(color.x()));
        bytes.push_back(to_byte(color.y()));
        bytes.push_back(to_byte(color.z()));
    }

    write_bytes(path, bytes);
}

}  // namespace rt
