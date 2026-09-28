//=============================================================================
// File:     hittable_list.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Closest-hit search over every shape in the scene.
//=============================================================================
#include "geometry/hittable_list.h"

#include <stdexcept>
#include <utility>

namespace rt {

void HittableList::add(std::unique_ptr<Hittable> object) {
    if (!object) {
        throw std::invalid_argument("Cannot add a null object to the scene");
    }
    objects_.push_back(std::move(object));
}

bool HittableList::hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const {
    // Pass the closest hit so far as the new upper bound. Each shape then
    // only reports a hit if it is nearer than everything seen before.
    HitRecord candidate;
    bool hit_anything = false;
    double closest_so_far = t_max;

    for (const auto& object : objects_) {
        if (object->hit(ray, t_min, closest_so_far, candidate)) {
            hit_anything = true;
            closest_so_far = candidate.t;
            record = candidate;
        }
    }
    return hit_anything;
}

Aabb HittableList::bounding_box() const {
    if (objects_.empty()) {
        throw std::logic_error("An empty scene has no bounding box");
    }
    Aabb box = objects_.front()->bounding_box();
    for (std::size_t i = 1; i < objects_.size(); ++i) {
        box = Aabb::surrounding(box, objects_[i]->bounding_box());
    }
    return box;
}

}  // namespace rt
