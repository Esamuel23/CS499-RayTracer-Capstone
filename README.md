# CS-499 Capstone — Ray Traced Scene (enhancement of a CS-330 artifact)

Taking a rasterized OpenGL scene built for SNHU CS-330 (Computer Graphics &
Visualization) and rebuilding its render path as a CPU ray tracer — reusing the
original artifact's scene construction and lighting work rather than starting
over, so the enhancement is measurable against a real "before."

**Status: in progress.** The plan is written; the implementation is being
written against it.

## Repository layout

```
original/     The CS-330 scene as it stood at the module 3 export - the "before" state
SPEC.md       The enhancement plan: what changes, in what order, and why
src/          The enhanced renderer (in progress)
```

## The original artifact

`original/` is the module 3 export (June 2025), not the finished course project —
CS-330 runs eight modules and the later scene work is not in this snapshot.

What is mine in it: `SceneManager.cpp` — roughly 285 lines written on top of the
module 3 starter, constructing the scene (a green ground plane, a dark box body,
and a loop-placed set of black box legs) with per-object transformations and
color material dispatch. `SceneManager.h`, `ShaderManager.*`, `ViewManager.cpp`
and `MainCode.cpp` carry smaller modifications over the course-provided versions.

What is course-provided and unmodified: `ShapeMeshes.cpp/.h` (~2,400 lines of
box/sphere/cylinder/cone/pyramid/plane mesh generators) and `Utilities/camera.h`.
Those are a dependency of the artifact, not authored work, and the enhancement
narrative should say so rather than counting their line volume as mine.

## The enhancement

Replace rasterization with ray tracing while keeping the scene definition and
material intent the original expressed:

- **Analytic ray-primitive intersection** for each shape type the scene uses,
  rather than tessellating everything to triangles.
- **Primary rays from the existing camera state**, reusing `ViewManager`'s
  position and basis vectors instead of a view matrix.
- **The original per-object transforms and colors**, carried into a ray-traced
  shading function rather than reinvented.
- **Shadow rays and recursive reflection** — the capability the raster version
  couldn't express without faking it.
- **A bounding volume hierarchy** (stretch), with before/after render timings.

Full staging and reasoning in [SPEC.md](SPEC.md).

## Building

The original project targets Visual Studio with GLEW, GLFW, and GLM. The
enhanced renderer has no graphics-API dependency — it writes images directly —
so the build for `src/` will be plain CMake. Build instructions land here once
there's something to build.
