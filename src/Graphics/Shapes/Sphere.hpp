#pragma once
#include <src/Graphics/IShape.hpp>

namespace devs_out_of_bounds {
class Sphere : public IShape {
public:
    Sphere(const glm::vec3& center, float radius) : m_center(center), m_radius2(radius * radius) {}

    DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
        const glm::vec3 oc = ray.origin - m_center;

        //const float a = glm::dot(ray.direction, ray.direction);
        const float b = 2.0f * glm::dot(ray.direction, oc);
        const float c = glm::dot(oc, oc) - m_radius2;

        const float discriminant = b * b - 4.0f * c;

        if (discriminant < 0.0f) {
            return false;
        }

        float t = (-b - glm::sqrt(discriminant)) * 0.5f;

        // If t0 is invalid (behind us or too far), try t1 (the exit point)
        if (t < ray.t_min || t > ray.t_max) {
            t = (-b + glm::sqrt(discriminant)) * 0.5f;
            
            // If t1 is ALSO invalid, then we truly missed (or are entirely behind/past it)
            if (t < ray.t_min || t > ray.t_max) {
                return false;
            }
            return false;
        }

        if (out_intersection) {
            out_intersection->t = t;
            out_intersection->normal = glm::normalize(oc + ray.direction * t);
            out_intersection->barycentric = { 0.0f, 0.0f }; // Spheres don't use barycentrics
            out_intersection->primitive = 0;
        }

        return true;
    }
    DOOB_NODISCARD AABB GetAABB() const override {
        float r = glm::sqrt(m_radius2);
        return AABB{
            .min = m_center - r,
            .max = m_center + r,
        };
    }

    glm::vec3 m_center;
    float m_radius2;
};
} // namespace devs_out_of_bounds