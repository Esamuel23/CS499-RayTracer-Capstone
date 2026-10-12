# Enhancement Two Spec: Bounding Volume Hierarchy (Algorithms and Data Structures)

Planning doc only. No implementation code. The C++ is yours to write.
Milestone Three wants: the enhancement, a narrative, and a zip of original + enhanced code.

---

## 0. The whole map (read this first)

```
TODAY (Enhancement One)                     AFTER (Enhancement Two)
-----------------------                     -----------------------
Ray ──► HittableList ──► test ALL n shapes  Ray ──► Bvh ──► test ~log n boxes ──► few shapes
        O(n) per ray                                O(log n) per ray (average)
```

| Piece | What it is | Where |
|---|---|---|
| `Bvh` | A `Hittable` that owns the shapes and a flat array of nodes | `src/accel/bvh.h/.cpp` |
| Build | Binned surface area heuristic (SAH), median split fallback | `Bvh` constructor |
| Traversal | Iterative, explicit stack, near child first, shrinking `t_max` | `Bvh::hit` |
| Switch | `--accel list|bvh` CLI flag, default `bvh` | `src/app/options.*` |
| Proof | Unit tests + list-vs-BVH image equality + benchmark CSV and chart | `tests/`, `bench/`, `docs/` |

Why it fits the category: it is a tree data structure, a recursive partitioning
algorithm, a cost-model heuristic, and a measured complexity change. Outcome 3
asks you to *evaluate* a solution and its trade-offs, and the benchmark is the evaluation.

---

## 1. Data structure

**Node (flattened, one contiguous `std::vector<Node>`):**

| Field | Meaning |
|---|---|
| `Aabb bounds` | Box around everything under this node |
| `uint32 left_or_first` | Interior: index of left child. Leaf: index of first primitive |
| `uint32 count` | 0 = interior node. >0 = leaf with this many primitives |

- Right child of an interior node is stored at `left_or_first + 1` (children adjacent),
  or store it explicitly. Pick one and write down why in the header comment.
- Primitives are reordered into a `std::vector<std::unique_ptr<Hittable>>` so each leaf
  is a contiguous range. Ownership stays `unique_ptr`, consistent with Enhancement One.

**Why flat array, not `new Node` pointers:** one allocation, cache-friendly, no manual
delete, trivially copyable/serializable. This is a trade-off paragraph for the narrative.

**Precompute per primitive at build time:** its `Aabb` and its centroid. Build never
calls `bounding_box()` more than once per shape.

---

## 2. Build algorithm (binned SAH)

Cost model (MacDonald & Booth, 1990; Wald, 2007):

```
cost(split) = C_trav + (SA(L)/SA(P)) * N_L * C_isect + (SA(R)/SA(P)) * N_R * C_isect
SA(box)     = 2 * (dx*dy + dy*dz + dz*dx)
```

Start with `C_trav = 1`, `C_isect = 1` (named constants, not literals).

Pseudocode:

```
build(range of primitives):
    node.bounds = union of primitive boxes in range
    if count <= kMaxLeafSize (4): make leaf, return

    centroid_bounds = box around the centroids in range
    axis = longest axis of centroid_bounds
    if centroid_bounds has zero extent on axis:     // all centroids stacked
        make leaf, return                           // or split in half by index

    for each of K bins (K = 12 or 16) along axis:
        bin.bounds, bin.count from primitives whose centroid falls in it
    sweep left-to-right and right-to-left to get SA and count for each of K-1 split planes
    best = split plane with lowest cost
    if best.cost >= leaf_cost (count * C_isect):    // splitting doesn't pay
        make leaf if count <= kMaxLeafFallback, else median split
    partition primitives in range around best plane (std::partition)
    if partition put everything on one side: median split by centroid (std::nth_element)
    node.left  = build(left half)
    node.right = build(right half)
```

Complexity to state in the narrative: build is O(n log n) for binned SAH;
query is O(log n) average, O(n) worst case (all boxes overlap).

Edge cases you must handle (each one is a test):
- 0 primitives → throw `std::invalid_argument` (mirror `HittableList::bounding_box`).
- 1 primitive → single leaf.
- All centroids identical → no infinite recursion.
- Flat boxes (the ground plane is ~0 thick on Y) → SA still > 0, no division by zero.

---

## 3. Traversal algorithm

```
hit(ray, t_min, t_max, record):
    if root bounds missed: return false
    stack = fixed array (depth 64), push root
    closest = t_max, found = false
    while stack not empty:
        node = pop
        if !node.bounds.hit(ray, t_min, closest): continue   // closest shrinks → prunes more
        if leaf:
            for each primitive: if prim.hit(ray, t_min, closest, temp): closest = temp.t, record = temp, found = true
        else:
            push FAR child first, NEAR child last (so near pops first)
            // near = the child whose box entry t is smaller, or use ray.direction sign on the split axis
    return found
```

Key correctness rule: pass `closest`, not the original `t_max`, into every box and
primitive test. That is what makes it return the *closest* hit, same as `HittableList`.

Guard the stack depth: assert / throw if the build ever produces depth > 64.

---

## 4. Tests to write (add to `tests/test_accel.cpp`)

| # | Test | Proves |
|---|---|---|
| 1 | Empty input throws | defensive input |
| 2 | Single sphere: BVH hit == list hit (t, point, normal) | base case |
| 3 | 1,000 random spheres, 10,000 random rays: BVH result == list result for every ray | correctness vs. trusted baseline |
| 4 | Ray that misses root box returns false, record untouched | contract |
| 5 | Root bounds == union of all primitive boxes | build invariant |
| 6 | Every child box is inside its parent box (walk the tree) | build invariant |
| 7 | Every primitive appears in exactly one leaf | no loss / duplication |
| 8 | All centroids identical (100 spheres at one point) builds and terminates | degenerate split |
| 9 | Desk scene: image with `--accel list` is byte-identical to `--accel bvh` | end-to-end |
| 10 | Max depth stays under the stack limit at 10,000 objects | traversal safety |

Use a fixed RNG seed for 3 and 8 so failures reproduce.

---

## 5. Benchmark plan (this is the Outcome 3 evidence)

Separate target `bench/bench_accel.cpp` → executable `raytracer_bench` (not run in CI).

- Scene: N random small spheres in a cube, fixed seed. N = 10, 100, 1,000, 10,000.
- For each N and each structure (list, bvh), record:
  - build time (ms)
  - render time (ms) at fixed resolution, 1 sample per pixel, 1 thread, max depth 1 (primary rays only, to isolate the data structure)
  - average box tests and primitive tests per ray (optional counters behind a compile flag so they cost nothing in normal builds)
- Repeat each run 5×, report the median.
- Output CSV: `docs/bench/bvh_results.csv` → chart `docs/bench/bvh_chart.png` (log-log axes; list should be a straight slope-1 line, BVH nearly flat).
- Record machine: CPU, compiler, build type. Put it under the chart.

Expected shape (fill with real numbers): list grows ~linearly with N; BVH grows ~log N;
crossover where BVH build cost stops paying back is probably at very small N. Report it honestly.

---

## 6. Integration

- `--accel list|bvh` flag (default `bvh`); validate strictly like the other options.
- Scenes build a `HittableList` as today; `main` hands its objects to `Bvh` when `--accel bvh`.
  Add a `HittableList::release_objects()` (or similar) so ownership moves, not copies.
- Add `src/accel/bvh.cpp` to `raytracer_core` in CMake; add `tests/test_accel.cpp` to the test target.
- Update README status table, "What changed" table, and add the chart.

---

## 7. Done when

- [ ] All 10 tests pass on Windows + Linux CI with warnings as errors
- [x] List and BVH images are byte-identical on the desk and showcase scenes
- [ ] Benchmark CSV + chart committed under `docs/bench/`
- [x] README updated (status, before/after, chart, how to run the benchmark)
- [x] Every new file has the standard header (name, date, purpose, design notes)
- [ ] Narrative placeholders filled with real numbers

## 8. Fallbacks if time gets tight

1. Drop binning → plain median split by longest axis (still O(log n) traversal; say so in the narrative).
2. Drop the per-ray counters → time only.
3. Drop near-first ordering → still correct, just slower; measure the difference if you add it later (good narrative material either way).

## References for the narrative

- MacDonald, J. D., & Booth, K. S. (1990). Heuristics for ray tracing using space subdivision. *The Visual Computer, 6*(3), 153–166. https://doi.org/10.1007/BF01911006
- Wald, I. (2007). On fast construction of SAH-based bounding volume hierarchies. *2007 IEEE Symposium on Interactive Ray Tracing*, 33–40. https://doi.org/10.1109/RT.2007.4342588
- Pharr, M., Jakob, W., & Humphreys, G. (2023). *Physically based rendering: From theory to implementation* (4th ed.). MIT Press. https://pbr-book.org/4ed/Primitives_and_Intersection_Acceleration/Bounding_Volume_Hierarchies
- Shirley, P., Black, T. D., & Hollasch, S. (2025). *Ray tracing: The next week* (Version 4.0.2). https://raytracing.github.io/books/RayTracingTheNextWeek.html
