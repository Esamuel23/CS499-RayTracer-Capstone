//=============================================================================
// File:     main.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Program entry point. Parse options, build the scene, render,
//           and write the image.
//
// Design notes:
//   The original MainCode.cpp kept four raw global pointers, deleted them
//   by hand, and skipped cleanup on some error paths. Here every object is
//   a local with automatic lifetime, and every failure reaches one catch
//   block that prints a message and returns a nonzero exit code.
//=============================================================================
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "app/options.h"
#include "render/camera.h"
#include "render/renderer.h"
#include "scene/desk_scene.h"

int main(int argc, char* argv[]) {
    try {
        const std::vector<std::string> arguments(argv + 1, argv + argc);
        const rt::app::Options options = rt::app::parse_options(arguments);

        if (options.show_help) {
            std::cout << rt::app::usage_text();
            return 0;
        }

        const auto world = (options.scene == rt::app::SceneChoice::Desk)
                               ? rt::scene::build_desk_scene()
                               : rt::scene::build_showcase_scene();

        const double aspect_ratio =
            static_cast<double>(options.render.width) / options.render.height;
        const rt::Camera camera((options.camera == rt::app::CameraChoice::Closeup)
                                    ? rt::scene::closeup_camera(aspect_ratio)
                                    : rt::scene::original_camera(aspect_ratio));
        const rt::Renderer renderer(options.render);

        std::cout << "Rendering " << world->size() << " objects at " << options.render.width
                  << "x" << options.render.height << ", " << options.render.samples_per_pixel
                  << " samples per pixel...\n";

        const auto start = std::chrono::steady_clock::now();
        const rt::Image image = renderer.render(*world, camera);
        const auto elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start);

        image.write(options.output_path);

        std::cout << "Done in " << elapsed.count() << " s. Wrote " << options.output_path
                  << "\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n\n" << rt::app::usage_text();
        return 1;
    }
}
