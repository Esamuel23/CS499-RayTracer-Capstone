//=============================================================================
// File:     material.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Abstract material plus three concrete materials.
//
// Design notes:
//   The original set material values as shader uniforms by string name
//   ("material.diffuseColor"), and the code review found that whole
//   subsystem was never called. Here a material is an object with one job:
//   decide how light scatters when a ray hits it. The renderer never checks
//   which material it has. It calls scatter() and virtual dispatch does
//   the rest.
//=============================================================================
#pragma once

#include "core/hittable.h"
#include "core/random.h"
#include "core/ray.h"
#include "core/vec3.h"

namespace rt {

class Material {
public:
    virtual ~Material() = default;

    // Decide what happens to a ray that hit this material.
    // Returns false when the ray is absorbed. Otherwise writes the
    // outgoing ray and how much each color channel is attenuated.
    virtual bool scatter(const Ray& ray_in, const HitRecord& record, Rng& rng,
                         Color& attenuation, Ray& scattered) const = 0;

protected:
    Material() = default;
    Material(const Material&) = default;
    Material& operator=(const Material&) = default;
};

// Matte surface. Scatters light in random directions around the normal.
class Lambertian final : public Material {
public:
    explicit Lambertian(const Color& albedo) : albedo_(albedo) {}

    bool scatter(const Ray& ray_in, const HitRecord& record, Rng& rng,
                 Color& attenuation, Ray& scattered) const override;

private:
    Color albedo_;
};

// Mirror-like surface. fuzz = 0 is a perfect mirror, 1 is brushed metal.
class Metal final : public Material {
public:
    // fuzz is clamped to [0, 1].
    Metal(const Color& albedo, double fuzz);

    bool scatter(const Ray& ray_in, const HitRecord& record, Rng& rng,
                 Color& attenuation, Ray& scattered) const override;

private:
    Color albedo_;
    double fuzz_;
};

// Clear material such as glass or water. Reflects or refracts.
class Dielectric final : public Material {
public:
    // Throws std::invalid_argument if refraction_index <= 0.
    explicit Dielectric(double refraction_index);

    bool scatter(const Ray& ray_in, const HitRecord& record, Rng& rng,
                 Color& attenuation, Ray& scattered) const override;

private:
    // Schlick's approximation of how much light reflects at an angle.
    static double reflectance(double cosine, double refraction_ratio);

    double refraction_index_;
};

}  // namespace rt
