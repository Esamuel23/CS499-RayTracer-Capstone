//=============================================================================
// File:     bvh.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     October 2026
// Purpose:  Builds the bounding volume hierarchy and walks it for each ray.
//
// Design notes:
//   Build cost model, the surface area heuristic (MacDonald and Booth, 1990;
//   Wald, 2007). The chance that a ray which hits a parent box also hits a
//   child box is about SA(child) / SA(parent). So the expected cost of a split
//   into L and R is
//
//     cost = C_trav + C_isect * (SA(L) * N_L + SA(R) * N_R) / SA(parent)
//
//   and the cost of not splitting (a leaf) is C_isect * N.
//
//   Binning (Wald, 2007). Instead of trying every possible split, centroids
//   are dropped into kBinCount equal-width bins along the longest axis, and
//   only the kBinCount - 1 planes between bins are scored. Two sweeps over
//   the bins (left to right, right to left) give the box and count on each
//   side of every plane, so one node costs O(n + bins) and the whole build
//   is O(n log n).
//
//   Bin count: 12. Wald (2007) reports that a few bins already come close to
//   a full SAH sweep and that 16 or more add little. PBRT (4th edition) also
//   uses 12. With at most 4 shapes per leaf and scenes up to 10,000 shapes,
//   12 bins (11 candidate planes) is enough; more bins mostly add build time.
//
//   Fallbacks, so the build always ends and never divides by zero:
//   - Every centroid in the same spot: split the range in half by position.
//   - Parent box with zero surface area (a line or a point): median split.
//   - Best split costs more than a leaf: keep a leaf if it is small
//     (kMaxLeafSizeNoSplit), otherwise median split.
//   - Partition puts every shape on one side: median split.
//   - At depth kSahMaxDepth and deeper the build uses only median splits.
//     A median split halves the count, so the tree can never outgrow the
//     traversal stack, even for scenes the SAH would split very unevenly.
//=============================================================================
#include "accel/bvh.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

#include "accel/traversal_stats.h"
#include "core/constants.h"

namespace rt {
namespace {

// Node indices are 32-bit. A tree over n shapes has at most 2n - 1 nodes,
// so n must stay below half the 32-bit range.
constexpr std::size_t kMaxPrimitives = std::numeric_limits<std::uint32_t>::max() / 2;

// Deepest node the build may create. Traversal holds at most depth + 1
// entries on its stack.
constexpr int kMaxTreeDepth = Bvh::kTraversalStackSize - 1;

// SAH cost constants. Only their ratio matters. 1 and 1 say a box test and
// a shape test cost about the same, the usual starting point.
constexpr double kTraversalCost = 1.0;
constexpr double kIntersectionCost = 1.0;

// Number of bins for the binned SAH. See the design notes above.
constexpr int kBinCount = 12;

// From this depth down, only median splits are used. A median split halves
// the shape count, and kMaxPrimitives < 2^31, so at most 31 more levels can
// follow: 32 + 31 = 63 = kMaxTreeDepth. The depth check in build() can then
// never fire.
constexpr int kSahMaxDepth = 32;
static_assert(kSahMaxDepth + 31 <= kMaxTreeDepth, "Median levels must fit in the stack");

// Everything the build needs to know about one shape, computed once.
struct PrimitiveInfo {
    Aabb box;
    Point3 centroid;
    std::uint32_t index;  // Position in the caller's original vector.
};

bool is_finite(const Aabb& box) {
    for (int axis = 0; axis < 3; ++axis) {
        if (!std::isfinite(box.min[axis]) || !std::isfinite(box.max[axis])) {
            return false;
        }
    }
    return true;
}

Point3 center_of(const Aabb& box) { return 0.5 * (box.min + box.max); }

// Axis (0 = x, 1 = y, 2 = z) along which the box is widest.
int longest_axis(const Aabb& box) {
    const Vec3 extent = box.max - box.min;
    if (extent.x() >= extent.y() && extent.x() >= extent.z()) {
        return 0;
    }
    return extent.y() >= extent.z() ? 1 : 2;
}

// Recursive builder. Works on index ranges [first, first + count) of the
// PrimitiveInfo array and appends nodes to the output vector.
class Builder {
public:
    Builder(std::vector<PrimitiveInfo>& infos, std::vector<Bvh::Node>& nodes, BvhSplit split)
        : infos_(infos), nodes_(nodes), split_(split) {}

    int max_depth() const { return max_depth_; }

    // Fills in nodes_[node_index] for the given range, then its subtree.
    // Uses indices, never references, into nodes_: the vector is reserved
    // up front, but an index stays valid even if it ever had to grow.
    void build(std::uint32_t node_index, std::uint32_t first, std::uint32_t count, int depth) {
        if (depth > kMaxTreeDepth) {
            throw std::length_error("BVH depth exceeds the traversal stack (" +
                                    std::to_string(Bvh::kTraversalStackSize) + " entries)");
        }
        max_depth_ = std::max(max_depth_, depth);

        Aabb bounds = infos_[first].box;
        for (std::uint32_t i = first + 1; i < first + count; ++i) {
            bounds = Aabb::surrounding(bounds, infos_[i].box);
        }
        nodes_[node_index].bounds = bounds;

        if (count <= Bvh::kMaxLeafSize) {
            make_leaf(node_index, first, count);
            return;
        }

        const Split split = choose_split(bounds, first, count, depth);
        if (split.make_leaf) {
            make_leaf(node_index, first, count);
            return;
        }
        const std::uint32_t mid = split.mid;

        // Children are always created as a pair, so right = left + 1.
        const auto left = static_cast<std::uint32_t>(nodes_.size());
        nodes_.emplace_back();
        nodes_.emplace_back();
        nodes_[node_index].left_or_first = left;
        nodes_[node_index].count = 0;

        build(left, first, mid - first, depth + 1);
        build(left + 1, mid, first + count - mid, depth + 1);
    }

private:
    // Either "make a leaf" or "split at mid" (mid strictly inside the range).
    struct Split {
        bool make_leaf;
        std::uint32_t mid;
    };

    struct Bin {
        Aabb bounds;
        std::uint32_t count = 0;
    };

    void make_leaf(std::uint32_t node_index, std::uint32_t first, std::uint32_t count) {
        nodes_[node_index].left_or_first = first;
        nodes_[node_index].count = count;
    }

    Aabb centroid_bounds(std::uint32_t first, std::uint32_t count) const {
        Aabb bounds{infos_[first].centroid, infos_[first].centroid};
        for (std::uint32_t i = first + 1; i < first + count; ++i) {
            bounds = Aabb::surrounding(bounds, Aabb{infos_[i].centroid, infos_[i].centroid});
        }
        return bounds;
    }

    // Puts the lower half of the centroids (along the longest axis) first.
    // Always returns a split point strictly inside the range, so both
    // children get at least one shape and the recursion always ends. When
    // every centroid is the same, this is simply a split by position.
    std::uint32_t median_split(std::uint32_t first, std::uint32_t count) {
        const int axis = longest_axis(centroid_bounds(first, count));
        const std::uint32_t mid = first + count / 2;
        const auto begin = infos_.begin() + first;
        std::nth_element(begin, infos_.begin() + mid, begin + count,
                         [axis](const PrimitiveInfo& a, const PrimitiveInfo& b) {
                             return a.centroid[axis] < b.centroid[axis];
                         });
        return mid;
    }

    // Bin that a centroid coordinate falls in. min_value and scale come from
    // the centroid bounds, so the result is always 0 to kBinCount - 1.
    static int bin_index(double value, double min_value, double scale) {
        const int bin = static_cast<int>((value - min_value) * scale);
        return std::min(std::max(bin, 0), kBinCount - 1);
    }

    Split choose_split(const Aabb& bounds, std::uint32_t first, std::uint32_t count, int depth) {
        const Aabb centroids = centroid_bounds(first, count);
        const int axis = longest_axis(centroids);
        const double min_value = centroids.min[axis];
        const double extent = centroids.max[axis] - min_value;

        // All centroids in one spot (the longest axis has zero length, so
        // every axis does). No plane can separate them. Split by position.
        if (!(extent > 0.0)) {
            return Split{false, first + count / 2};
        }

        const double parent_area = bounds.surface_area();
        if (split_ == BvhSplit::Median || depth >= kSahMaxDepth || !(parent_area > 0.0)) {
            return Split{false, median_split(first, count)};
        }

        // A centroid spread so tiny that kBinCount / extent overflows cannot
        // be binned safely (inf * 0 is NaN, and NaN cannot become an int).
        const double scale = kBinCount / extent;
        if (!std::isfinite(scale)) {
            return Split{false, median_split(first, count)};
        }

        // Drop every centroid into a bin and grow that bin's box.
        std::array<Bin, kBinCount> bins{};
        for (std::uint32_t i = first; i < first + count; ++i) {
            Bin& bin = bins[static_cast<std::size_t>(bin_index(infos_[i].centroid[axis], min_value, scale))];
            bin.bounds = (bin.count == 0) ? infos_[i].box : Aabb::surrounding(bin.bounds, infos_[i].box);
            ++bin.count;
        }

        // Plane p lies between bin p and bin p + 1. Sweep from the left to
        // get the area and count of everything left of each plane, then from
        // the right for everything right of it.
        constexpr int kPlaneCount = kBinCount - 1;
        std::array<double, kPlaneCount> left_area{};
        std::array<std::uint32_t, kPlaneCount> left_count{};
        std::array<double, kPlaneCount> right_area{};
        std::array<std::uint32_t, kPlaneCount> right_count{};

        Bin running;
        for (int p = 0; p < kPlaneCount; ++p) {
            add_bin(running, bins[static_cast<std::size_t>(p)]);
            left_count[static_cast<std::size_t>(p)] = running.count;
            left_area[static_cast<std::size_t>(p)] = running.count > 0 ? running.bounds.surface_area() : 0.0;
        }
        running = Bin{};
        for (int p = kPlaneCount - 1; p >= 0; --p) {
            add_bin(running, bins[static_cast<std::size_t>(p + 1)]);
            right_count[static_cast<std::size_t>(p)] = running.count;
            right_area[static_cast<std::size_t>(p)] = running.count > 0 ? running.bounds.surface_area() : 0.0;
        }

        // Score every plane that leaves shapes on both sides.
        int best_plane = -1;
        double best_cost = kInfinity;
        for (std::size_t p = 0; p < static_cast<std::size_t>(kPlaneCount); ++p) {
            if (left_count[p] == 0 || right_count[p] == 0) {
                continue;
            }
            const double cost = kTraversalCost +
                                kIntersectionCost *
                                    (left_area[p] * left_count[p] + right_area[p] * right_count[p]) /
                                    parent_area;
            if (cost < best_cost) {
                best_cost = cost;
                best_plane = static_cast<int>(p);
            }
        }
        if (best_plane < 0) {
            return Split{false, median_split(first, count)};
        }

        // Splitting is not worth it. A small group stays a leaf. A big one
        // is split at the median anyway so no leaf gets too large.
        const double leaf_cost = kIntersectionCost * count;
        if (best_cost >= leaf_cost) {
            if (count <= Bvh::kMaxLeafSizeNoSplit) {
                return Split{true, 0};
            }
            return Split{false, median_split(first, count)};
        }

        // Move shapes left of the plane to the front of the range. The same
        // bin_index() call is used, so every shape lands on the side it was
        // counted on.
        const auto begin = infos_.begin() + first;
        const auto middle = std::partition(begin, begin + count, [&](const PrimitiveInfo& info) {
            return bin_index(info.centroid[axis], min_value, scale) <= best_plane;
        });
        const auto mid = static_cast<std::uint32_t>(middle - infos_.begin());
        if (mid == first || mid == first + count) {
            return Split{false, median_split(first, count)};
        }
        return Split{false, mid};
    }

    static void add_bin(Bin& running, const Bin& bin) {
        if (bin.count == 0) {
            return;
        }
        running.bounds = (running.count == 0) ? bin.bounds : Aabb::surrounding(running.bounds, bin.bounds);
        running.count += bin.count;
    }

    std::vector<PrimitiveInfo>& infos_;
    std::vector<Bvh::Node>& nodes_;
    BvhSplit split_;
    int max_depth_ = 0;
};

}  // namespace

Bvh::Bvh(std::vector<std::unique_ptr<Hittable>> objects, BvhSplit split) {
    if (objects.empty()) {
        throw std::invalid_argument("Cannot build a BVH with no objects");
    }
    if (objects.size() > kMaxPrimitives) {
        throw std::length_error("Too many objects for a BVH with 32-bit indices");
    }

    // Each shape's box and centroid, computed exactly once.
    std::vector<PrimitiveInfo> infos;
    infos.reserve(objects.size());
    for (std::size_t i = 0; i < objects.size(); ++i) {
        if (!objects[i]) {
            throw std::invalid_argument("Cannot build a BVH that contains a null object");
        }
        const Aabb box = objects[i]->bounding_box();
        if (!is_finite(box)) {
            throw std::invalid_argument("Object bounding box must be finite to build a BVH");
        }
        infos.push_back(PrimitiveInfo{box, center_of(box), static_cast<std::uint32_t>(i)});
    }

    const auto count = static_cast<std::uint32_t>(infos.size());
    nodes_.reserve(2 * static_cast<std::size_t>(count) - 1);  // Upper bound for a binary tree.
    nodes_.emplace_back();

    Builder builder(infos, nodes_, split);
    builder.build(0, 0, count, 0);
    depth_ = builder.max_depth();
    nodes_.shrink_to_fit();

    // Move the shapes into leaf order so every leaf is one contiguous range.
    primitives_.reserve(infos.size());
    for (const PrimitiveInfo& info : infos) {
        primitives_.push_back(std::move(objects[info.index]));
    }
}

bool Bvh::hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const {
    // A stack entry remembers where the ray enters the node's box. If a
    // closer hit is found after the node was pushed, the node is skipped
    // without testing it again.
    struct StackEntry {
        std::uint32_t node;
        double t_enter;
    };

    double root_enter = 0.0;
    count_box_test();
    if (!nodes_.front().bounds.hit(ray, t_min, t_max, root_enter)) {
        return false;  // Record untouched, same contract as every Hittable.
    }

    std::array<StackEntry, kTraversalStackSize> stack;
    std::size_t size = 0;
    stack[size++] = StackEntry{0, root_enter};

    HitRecord candidate;
    bool hit_anything = false;
    double closest = t_max;

    while (size > 0) {
        const StackEntry entry = stack[--size];
        if (entry.t_enter >= closest) {
            continue;  // Everything in this box is farther than the current hit.
        }

        const Node& node = nodes_[entry.node];
        if (node.is_leaf()) {
            // Same closest-hit loop as HittableList, over this leaf's range.
            const std::uint32_t end = node.left_or_first + node.count;
            for (std::uint32_t i = node.left_or_first; i < end; ++i) {
                count_primitive_test();
                if (primitives_[i]->hit(ray, t_min, closest, candidate)) {
                    hit_anything = true;
                    closest = candidate.t;
                    record = candidate;
                }
            }
            continue;
        }

        // Interior node: test both children now so we know which is nearer.
        const std::uint32_t left = node.left_or_first;
        const std::uint32_t right = left + 1;
        double left_enter = 0.0;
        double right_enter = 0.0;
        count_box_test();
        const bool hit_left = nodes_[left].bounds.hit(ray, t_min, closest, left_enter);
        count_box_test();
        const bool hit_right = nodes_[right].bounds.hit(ray, t_min, closest, right_enter);

        // The build caps the depth, so the stack cannot overflow. The
        // assert documents and checks that promise in debug builds.
        assert(size + 2 <= stack.size());
        if (hit_left && hit_right) {
            // Push the far child first so the near child is popped first.
            if (left_enter <= right_enter) {
                stack[size++] = StackEntry{right, right_enter};
                stack[size++] = StackEntry{left, left_enter};
            } else {
                stack[size++] = StackEntry{left, left_enter};
                stack[size++] = StackEntry{right, right_enter};
            }
        } else if (hit_left) {
            stack[size++] = StackEntry{left, left_enter};
        } else if (hit_right) {
            stack[size++] = StackEntry{right, right_enter};
        }
    }
    return hit_anything;
}

Aabb Bvh::bounding_box() const { return nodes_.front().bounds; }

const Hittable& Bvh::primitive(std::size_t index) const {
    if (index >= primitives_.size()) {
        throw std::out_of_range("BVH primitive index out of range");
    }
    return *primitives_[index];
}

}  // namespace rt
