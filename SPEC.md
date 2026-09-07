# CS-499 Capstone — Enhancement Plan: CS-330 Scene → Ray Tracer

This is a planning document only. No implementation included on purpose —
same pattern as RayTracer/SPEC.md and LogEngine/SPEC.md: the design and
code are yours to write, this just scopes the work and the narrative.

**Caveat:** this is built from general knowledge of how SNHU's CS-499
capstone is structured (three enhancement categories: software design &
engineering, algorithms & data structures, databases; a narrative per
enhancement; a professional self-assessment; a GitHub-hosted ePortfolio).
Check your actual course materials/rubric against this before committing
to the plan — categories, deliverable format, or milestone dates may
differ from what's assumed here.

## The artifact being enhanced

Source (untouched, still where it was submitted):
`SNHU projects/M3exportCS330/` — the **module 3 export**, June 2025.

- `SceneManager.cpp` / `.h` — the scene: ground plane, box body, loop-placed
  legs, with transform and color dispatch. This is the authored work (~285 lines
  over the module 3 starter).
- `ShaderManager.cpp` / `.h`, `ViewManager.cpp` / `.h`, `MainCode.cpp` — lightly
  modified course-provided plumbing.
- `ShapeMeshes.cpp` / `.h`, `Utilities/camera.h` — course-provided and
  unmodified. A dependency, not authored work.

Original artifact type: SNHU CS-330 (Computer Graphics), rasterized OpenGL.

**Gap to close first:** CS-330 runs eight modules, and this snapshot stops at
module 3 — the textured, fully lit final scene from the later modules is not on
this machine. Recover the module 8 / final project submission (Brightspace keeps
submitted files, or pull it off the old PC) and swap it into `original/` before
building on this. A ground plane and three boxes is a thin "before" for a
capstone enhancement narrative; the final scene is a much stronger one.

## The enhancement story

Don't rebuild from zero — that's not what CS-499 wants, and it throws
away the part of the narrative that shows growth on *existing* work. The
enhancement is: replace the rasterization pipeline with a ray-traced one
while deliberately reusing what the original artifact already got right —
the shape/geometry definitions and the lighting model — so the writeup
can point at specific old code and say "this is what changed and why."

This most likely covers two of the three categories in one artifact:
- **Software design & engineering** — replacing a rasterization
  architecture with a ray-tracing one: new `Ray`, `Hittable`, `Material`
  abstractions; clean separation from the old immediate-mode OpenGL calls.
- **Algorithms & data structures** — ray-primitive intersection math per
  shape type, recursive ray bounces for reflection/refraction, and
  (stretch) a bounding-volume hierarchy so ray/object tests aren't O(n)
  against every object in the scene.

(The databases category is a separate, third artifact — the CS-340
Animal Shelter/MongoDB project is the natural candidate; not covered in
this doc.)

## Build sequence

1. **Extract geometry, not vertices.** The five shapes in `ShapeMeshes`
   are generated proceedurally (box, sphere, cylinder, cone, pyramid,
   plane) — you know their parametric definitions already existed before
   they were turned into vertex buffers. Write analytic ray-intersection
   formulas per shape type (ray-sphere is the easy one; ray-box via slab
   test; cylinder/cone are the two that will eat the most time) rather
   than converting every mesh to triangles and testing per-triangle. Keep
   a note in the writeup on why you chose analytic over triangle-mesh
   intersection — that's a design-tradeoff paragraph the rubric likely
   wants.
2. **Camera.** Reuse `ViewManager`'s existing camera position/orientation
   state to generate primary rays (one ray per pixel through the camera's
   existing basis vectors) instead of feeding it into a view matrix.
3. **Port the lighting model.** `SceneManager::SetShaderMaterial` and the
   existing light setup already encode ambient/diffuse/specular
   parameters per object. Reuse those material values in the ray tracer's
   shading function instead of reinventing material parameters — this is
   the clearest "enhanced, not replaced" evidence for the narrative.
4. **Shadows.** Shadow rays from each hit point toward each light —
   natural next step once basic shading works, and a good before/after
   screenshot (flat-shaded vs. shadowed).
5. **Reflection.** Recursive bounce for any material marked reflective —
   pick 1-2 objects in the original scene to make mirrored as the
   showcase image.
6. **Stretch — acceleration structure.** If you want the strongest
   algorithms-category story: add a BVH or simple spatial grid, and
   benchmark render time before/after on a scene with many objects. A
   real "O(n) naive → O(log n) accelerated" number with a chart is a
   strong self-assessment data point, not just a claim.

## What to bring over

Copy (don't move) these into wherever the new capstone repo lives, so the
original coursework submission stays untouched:
- `ShapeMeshes.cpp` / `.h` (the 2,446-line version)
- `SceneManager.cpp` / `.h`
- `ViewManager.cpp` / `.h`
- Whatever shader files `7-1_FinalProjectMilestones` references

## Repo / portfolio

Keep this **separate** from `-No-AI-Raytracing` — that repo's own
description commits it to a no-AI, from-scratch challenge, and mixing a
course deliverable into it muddies both stories. Suggest a fresh repo,
e.g. `cs499-raytracer-enhancement`, public (the ePortfolio requirement
needs a public link), with:
- The original artifact linked/described in the README for contrast
- A `narrative.md` or PDF per SNHU's required enhancement-narrative format
- Before/after renders (flat rasterized scene vs. ray-traced result) —
  these double as Portfolio site assets (`DevPractice/Portfolio` already
  has placeholder slots for exactly this kind of screenshot swap)

## Notes

- Confirm against your instructor's actual rubric before treating the
  three-category breakdown above as fixed — this doc is scoped from
  general knowledge of the program structure, not your syllabus.
- Nothing here is timed against your October deadline — pace stages 1-5
  first; stage 6 (BVH) is a stretch, not a requirement, if time gets
  tight.
