//=============================================================================
// File:     bvh.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     October 2026
// Purpose:  Bounding volume hierarchy: a tree of boxes that lets a ray skip
//           every shape whose box it misses.
//
// Design notes:
//   HittableList tests every shape for every ray, which is O(n) per ray.
//   The Bvh is also a Hittable, so the renderer uses it without any change,
//   but a ray only visits the boxes it actually passes through: O(log n)
//   per ray on average, O(n) in the worst case (every box overlaps).
//
//   Data structure. All nodes live in one std::vector<Node>, not as
//   separately allocated nodes joined by pointers. One allocation, nodes
//   sit next to each other in memory (cache friendly), nothing to delete by
//   hand, and the tree can be copied or saved as plain data.
//   A node is a box plus two numbers. count == 0 means an interior node and
//   left_or_first is the index of its left child. count > 0 means a leaf
//   and left_or_first is the index of its first shape.
//   The right child is always stored at left_or_first + 1. The builder
//   creates both children together, so storing the right index as well
//   would spend 4 bytes per node on a number that is always left + 1.
//
//   The shapes are moved into one std::vector<std::unique_ptr<Hittable>>
//   and reordered so every leaf owns a contiguous range. Ownership stays
//   unique_ptr, the same rule as the rest of the project.
//
//   Build. Binned surface area heuristic (SAH) by default. Each primitive's
//   box and centroid are computed once, up front. See bvh.cpp for the cost
//   model and the fallbacks. A plain median split is also available, both
//   as the fallback and so the benchmark can compare the two.
//
//   Traversal. Iterative, with a fixed-size stack instead of recursion.
//   The nearer child is visited first and the closest hit so far is passed
//   into every box and shape test, so far boxes are skipped once something
//   closer is found. The build refuses to make a tree deeper than the stack
//   can hold.
//=============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "core/aabb.h"
#include "core/hittable.h"

namespace rt {

// How the builder chooses where to split a node.
enum class BvhSplit {
    Sah,    // Binned surface area heuristic. Default. Fewer tests per ray.
    Median  // Half the shapes on each side along the longest axis. Simple.
};

class Bvh final : public Hittable {
public:
    struct Node {
        Aabb bounds;                     // Box around everything below this node.
        std::uint32_t left_or_first = 0; // Interior: left child. Leaf: first shape.
        std::uint32_t count = 0;         // 0 = interior node, > 0 = shapes in leaf.

        bool is_leaf() const { return count > 0; }
    };

    // Leaves hold at most this many shapes, except when the SAH says a
    // split costs more than testing the shapes (see kMaxLeafSizeNoSplit).
    static constexpr std::uint32_t kMaxLeafSize = 4;

    // Largest leaf the SAH may keep when no split pays for itself. Bigger
    // groups are split at the median anyway, so one leaf can never hold a
    // large part of the scene.
    static constexpr std::uint32_t kMaxLeafSizeNoSplit = 16;

    // Entries in the traversal stack. Traversal needs at most depth + 1
    // entries, so the build rejects any tree deeper than this minus one.
    static constexpr int kTraversalStackSize = 64;

    // Takes ownership of every shape and builds the tree.
    // Throws std::invalid_argument if objects is empty, holds a null pointer,
    // or a shape reports a box with a NaN or infinite coordinate.
    // Throws std::length_error if there are too many shapes to index with
    // 32-bit numbers, or if the tree would be deeper than the stack.
    explicit Bvh(std::vector<std::unique_ptr<Hittable>> objects,
                 BvhSplit split = BvhSplit::Sah);

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const override;

    // Box around the whole scene (the root node's box).
    Aabb bounding_box() const override;

    // Read-only views used by the tests and the benchmark.
    const std::vector<Node>& nodes() const { return nodes_; }
    std::size_t primitive_count() const { return primitives_.size(); }
    const Hittable& primitive(std::size_t index) const;
    int depth() const { return depth_; }  // Root is depth 0.

private:
    std::vector<Node> nodes_;
    std::vector<std::unique_ptr<Hittable>> primitives_;
    int depth_ = 0;
};

}  // namespace rt
