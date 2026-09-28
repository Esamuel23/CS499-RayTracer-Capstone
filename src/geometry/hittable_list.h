//=============================================================================
// File:     hittable_list.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  A collection of shapes that is itself a Hittable (the scene).
//
// Design notes:
//   The list owns its shapes through std::unique_ptr. Nothing in the scene
//   is deleted by hand. This replaces the original's raw new and delete,
//   which the code review flagged for leaks and a rule-of-three problem.
//
//   Each ray is tested against every shape: O(n) per ray. That is the
//   baseline Enhancement Two will replace with a bounding volume hierarchy.
//=============================================================================
#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/hittable.h"

namespace rt {

class HittableList final : public Hittable {
public:
    HittableList() = default;

    // Takes ownership. Throws std::invalid_argument on a null pointer.
    void add(std::unique_ptr<Hittable> object);

    std::size_t size() const { return objects_.size(); }
    bool empty() const { return objects_.empty(); }

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const override;

    // Throws std::logic_error when the list is empty (no box exists).
    Aabb bounding_box() const override;

private:
    std::vector<std::unique_ptr<Hittable>> objects_;
};

}  // namespace rt
