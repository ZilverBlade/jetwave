#pragma once
#include <src/Graphics/IShape.hpp>

namespace devs_out_of_bounds {
class Sphere : public IShape {
public:
    Sphere(const glm::vec3& center, float radius) : m_center(center), m_radius2(radius * radius) {}

    DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {

        const glm::vec3 origin = ray.origin - m_center;
        // const float A = glm::dot(ray_direction, ray_direction);
        const float B = 2.0f * glm::dot(ray.direction, origin);
        const float C = glm::dot(origin, origin) - m_radius2;

        float discriminant = B * B - 4.0f * C;
        if (discriminant < 0.0f) {
            return false;
        }
        const float two_t0 = -B - glm::sqrt(discriminant);
        // if it's behind us, do not intersect.
        if (two_t0 < 0.0f) {
            // Sidenote: this also does backface culling, if there are 2 intersections,
            // dont care and just take the closest
            return false;
        }
        if (out_intersection) {
            out_intersection->t = two_t0 * 0.5f;
            out_intersection->normal = glm::normalize(ray.direction * out_intersection->t + origin);
            out_intersection->barycentric = { 0.0f, 0.0f };
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