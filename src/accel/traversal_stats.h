//=============================================================================
// File:     traversal_stats.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     October 2026
// Purpose:  Optional counters for box tests and primitive tests per ray.
//
// Design notes:
//   The benchmark reports how many bounding box tests and shape tests each
//   ray costs. That is the clearest evidence of O(n) versus O(log n), and it
//   does not depend on the machine. Counting has a price, so it is compiled
//   in only when RT_ENABLE_TRAVERSAL_STATS is defined (the bench target sets
//   it on its own copy of the library). In every other build the count
//   functions are empty: the increment sits behind "if constexpr (false)",
//   so no code is generated for it.
//
//   The renderer runs on worker threads even when asked for one thread, so
//   the counters are relaxed atomics. Any thread can add to them and the
//   totals are correct after the render joins its workers.
//=============================================================================
#pragma once

#include <atomic>
#include <cstdint>

namespace rt {

#if defined(RT_ENABLE_TRAVERSAL_STATS)
inline constexpr bool kTraversalStatsEnabled = true;
#else
inline constexpr bool kTraversalStatsEnabled = false;
#endif

// Snapshot of the counters.
struct TraversalStats {
    std::uint64_t box_tests = 0;        // Aabb tests made by the BVH.
    std::uint64_t primitive_tests = 0;  // Shape hit() calls by a list or BVH leaf.
};

namespace detail {
inline std::atomic<std::uint64_t> g_box_tests{0};
inline std::atomic<std::uint64_t> g_primitive_tests{0};
}  // namespace detail

inline void count_box_test() {
    if constexpr (kTraversalStatsEnabled) {
        detail::g_box_tests.fetch_add(1, std::memory_order_relaxed);
    }
}

inline void count_primitive_test() {
    if constexpr (kTraversalStatsEnabled) {
        detail::g_primitive_tests.fetch_add(1, std::memory_order_relaxed);
    }
}

// Both are safe to call in any build. Without the flag the counters are
// always zero.
inline TraversalStats traversal_stats() {
    TraversalStats stats;
    stats.box_tests = detail::g_box_tests.load(std::memory_order_relaxed);
    stats.primitive_tests = detail::g_primitive_tests.load(std::memory_order_relaxed);
    return stats;
}

inline void reset_traversal_stats() {
    detail::g_box_tests.store(0, std::memory_order_relaxed);
    detail::g_primitive_tests.store(0, std::memory_order_relaxed);
}

}  // namespace rt
