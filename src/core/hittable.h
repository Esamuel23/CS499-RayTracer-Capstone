//=============================================================================
// File:     hittable.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Abstract interface for anything a ray can hit.
//
// Design notes:
//   In the original artifact each shape was a separate Draw...Mesh() call,
//   and RenderScene had to know every shape type. Here every shape
//   implements one interface. The renderer only talks to Hittable, so a new
//   shape can be added without changing the render loop (open/closed
//   principle).
//=============================================================================
#pragma once

#include "core/aabb.h"
#include "core/ray.h"
#include "core/vec3.h"

namespace rt {

class Material;

// Everything the renderer needs to know about one ray-surface hit.
struct HitRecord {
    Point3 point;
    Vec3 normal;                         // Unit length, always faces the ray.
    double t = 0.0;                      // Ray parameter at the hit.
    bool front_face = false;             // True when the ray hit the outside.
    const Material* material = nullptr;  // Non-owning. The shape owns it.

    // Store the normal so it always points against the incoming ray, and
    // remember which side was hit. Glass needs that side to pick the
    // correct refraction ratio.
    void set_face_normal(const Ray& ray, const Vec3& outward_normal) {
        front_face = dot(ray.direction(), outward_normal) < 0.0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Hittable {
public:
    virtual ~Hittable() = default;

    // Reports the closest hit with t strictly inside (t_min, t_max).
    // Returns false and leaves the record untouched on a miss.
    virtual bool hit(const Ray& ray, double t_min, double t_max,
                     HitRecord& record) const = 0;

    // Box that fully contains the shape.
    virtual Aabb bounding_box() const = 0;

protected:
    Hittable() = default;
    Hittable(const Hittable&) = default;
    Hittable& operator=(const Hittable&) = default;
};

}  // namespace rt
