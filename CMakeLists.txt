#=============================================================================
# File:     CMakeLists.txt
# Author:   Elijah Samuel
# Course:   CS 499 Computer Science Capstone, SNHU
# Date:     September 2026 (bench target and BVH added October 2026)
# Purpose:  Portable build for the ray tracer, its tests, its benchmark,
#           and CI.
#
# The original artifact only built inside one Visual Studio solution with
# GLEW, GLFW, and GLM installed by hand. This build needs only a C++17
# compiler and CMake, and runs the same on Windows, Linux, and macOS.
#
#   cmake -S . -B build
#   cmake --build build --config Release
#   ctest --test-dir build -C Release --output-on-failure
#
# The benchmark (raytracer_bench) is built but never run by CTest or CI.
# Run it by hand on a Release build. See README.md.
#=============================================================================
cmake_minimum_required(VERSION 3.16)
project(CS499RayTracer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ray tracing is slow without optimization. Default to Release for
# single-config generators (Makefiles, Ninja).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(RT_BUILD_TESTS "Build the unit tests" ON)
option(RT_WARNINGS_AS_ERRORS "Treat compiler warnings as errors (used in CI)" OFF)
option(RT_BUILD_BENCH "Build the BVH benchmark (never run in CI)" ON)

find_package(Threads REQUIRED)

# Strict warnings on every target. CI turns them into errors.
function(rt_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(RT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
        if(RT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# Everything except main() lives in a library, so the tests link the exact
# code the application runs.
set(RT_CORE_SOURCES
    src/accel/bvh.cpp
    src/core/aabb.cpp
    src/geometry/box.cpp
    src/geometry/cone.cpp
    src/geometry/cylinder.cpp
    src/geometry/hittable_list.cpp
    src/geometry/plane.cpp
    src/geometry/sphere.cpp
    src/materials/material.cpp
    src/render/camera.cpp
    src/render/image.cpp
    src/render/renderer.cpp
    src/scene/desk_scene.cpp
    src/app/options.cpp
)
add_library(raytracer_core STATIC ${RT_CORE_SOURCES})
target_include_directories(raytracer_core PUBLIC src)
target_link_libraries(raytracer_core PUBLIC Threads::Threads)
rt_set_warnings(raytracer_core)

add_executable(raytracer src/app/main.cpp)
target_link_libraries(raytracer PRIVATE raytracer_core)
rt_set_warnings(raytracer)

if(RT_BUILD_TESTS)
    enable_testing()

    add_executable(raytracer_tests
        tests/test_main.cpp
        tests/test_math.cpp
        tests/test_geometry.cpp
        tests/test_render.cpp
        tests/test_accel.cpp
    )
    target_link_libraries(raytracer_tests PRIVATE raytracer_core)
    rt_set_warnings(raytracer_tests)

    add_test(NAME unit_tests COMMAND raytracer_tests)

    # End-to-end check: the program runs and writes a real file.
    add_test(NAME smoke_render
        COMMAND raytracer --scene showcase --width 64 --height 48 --samples 2 --out smoke.bmp)

    # Bad input must fail with a nonzero exit code, not crash or continue.
    add_test(NAME rejects_bad_arguments COMMAND raytracer --width 0)
    set_tests_properties(rejects_bad_arguments PROPERTIES WILL_FAIL TRUE)
    add_test(NAME rejects_bad_accel COMMAND raytracer --accel kdtree)
    set_tests_properties(rejects_bad_accel PROPERTIES WILL_FAIL TRUE)

    # Enhancement Two end-to-end check: the same scene rendered with the
    # list and with the BVH must produce byte-identical files.
    set(RT_ACCEL_ARGS --scene showcase --width 64 --height 48 --samples 4 --depth 8)
    add_test(NAME render_with_list COMMAND raytracer ${RT_ACCEL_ARGS} --accel list --out accel_list.bmp)
    add_test(NAME render_with_bvh COMMAND raytracer ${RT_ACCEL_ARGS} --accel bvh --out accel_bvh.bmp)
    set_tests_properties(render_with_list render_with_bvh PROPERTIES FIXTURES_SETUP accel_renders)
    add_test(NAME list_and_bvh_images_identical
        COMMAND ${CMAKE_COMMAND} -E compare_files accel_list.bmp accel_bvh.bmp)
    set_tests_properties(list_and_bvh_images_identical PROPERTIES FIXTURES_REQUIRED accel_renders)
endif()

if(RT_BUILD_BENCH)
    # Benchmark: list versus BVH on growing random scenes. Built so it
    # cannot rot, but never run by CTest or CI (it takes about a minute and
    # its timings depend on the machine).
    add_executable(raytracer_bench bench/bench_accel.cpp)
    target_link_libraries(raytracer_bench PRIVATE raytracer_core)
    rt_set_warnings(raytracer_bench)

    # Same library and benchmark with the per-ray counters compiled in.
    # Counting slows every test down, so timings always come from
    # raytracer_bench and only the counts come from this build.
    add_library(raytracer_core_stats STATIC ${RT_CORE_SOURCES})
    target_include_directories(raytracer_core_stats PUBLIC src)
    target_compile_definitions(raytracer_core_stats PUBLIC RT_ENABLE_TRAVERSAL_STATS)
    target_link_libraries(raytracer_core_stats PUBLIC Threads::Threads)
    rt_set_warnings(raytracer_core_stats)

    add_executable(raytracer_bench_stats bench/bench_accel.cpp)
    target_link_libraries(raytracer_bench_stats PRIVATE raytracer_core_stats)
    rt_set_warnings(raytracer_bench_stats)
endif()
