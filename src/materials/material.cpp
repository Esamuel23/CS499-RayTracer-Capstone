//=============================================================================
// File:     material.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Scatter behavior for matte, metal, and glass materials.
//=============================================================================
#include "materials/material.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace rt {

//----------------------------------------------------------------------------
// Lambertian
//----------------------------------------------------------------------------
bool Lambertian::scatter(const Ray& /*ray_in*/, const HitRecord& record, Rng& rng,
                         Color& attenuation, Ray& scattered) const {
    // Normal plus a random unit vector gives a cosine-weighted direction,
    // which is how an ideal matte surface spreads light.
    Vec3 direction = record.normal + rng.unit_vector();

    // If the random vector nearly cancels the normal, the sum is almost
    // zero and would produce a bad ray. Fall back to the normal.
    if (direction.near_zero()) {
        direction = record.normal;
    }

    scattered = Ray(record.point, direction);
    attenuation = albedo_;
    return true;
}

//----------------------------------------------------------------------------
// Metal
//----------------------------------------------------------------------------
Metal::Metal(const Color& albedo, double fuzz)
    : albedo_(albedo), fuzz_(std::clamp(fuzz, 0.0, 1.0)) {}

bool Metal::scatter(const Ray& ray_in, const HitRecord& record, Rng& rng,
                    Color& attenuation, Ray& scattered) const {
    const Vec3 reflected = reflect(unit_vector(ray_in.direction()), record.normal);
    scattered = Ray(record.point, reflected + fuzz_ * rng.unit_vector());
    attenuation = albedo_;

    // Fuzz can push the ray below the surface. Treat that ray as absorbed.
    return dot(scattered.direction(), record.normal) > 0.0;
}

//----------------------------------------------------------------------------
// Dielectric
//----------------------------------------------------------------------------
Dielectric::Dielectric(double refraction_index) : refraction_index_(refraction_index) {
    if (!(refraction_index_ > 0.0)) {
        throw std::invalid_argument("Refraction index must be positive");
    }
}

bool Dielectric::scatter(const Ray& ray_in, const HitRecord& record, Rng& rng,
                         Color& attenuation, Ray& scattered) const {
    attenuation = Color(1.0, 1.0, 1.0);  // Clear glass absorbs nothing.

    // Entering the glass from air, or leaving it back into air.
    const double ratio = record.front_face ? (1.0 / refraction_index_) : refraction_index_;

    const Vec3 unit_direction = unit_vector(ray_in.direction());
    const double cos_theta = std::fmin(dot(-unit_direction, record.normal), 1.0);
    const double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

    // Total internal reflection: Snell's law has no solution, so the ray
    // must reflect. Otherwise reflect with the Fresnel probability.
    const bool cannot_refract = ratio * sin_theta > 1.0;
    const Vec3 direction = (cannot_refract || reflectance(cos_theta, ratio) > rng.next())
                               ? reflect(unit_direction, record.normal)
                               : refract(unit_direction, record.normal, ratio);

    scattered = Ray(record.point, direction);
    return true;
}

double Dielectric::reflectance(double cosine, double refraction_ratio) {
    double r0 = (1.0 - refraction_ratio) / (1.0 + refraction_ratio);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * std::pow(1.0 - cosine, 5.0);
}

}  // namespace rt
