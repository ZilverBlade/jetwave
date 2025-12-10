#pragma once
#include <src/Graphics/IShape.hpp>

namespace devs_out_of_bounds {
class Plane : public IShape {
public:
    Plane(const glm::vec3& normal, const glm::vec3& position) : m_normal(normal), m_d(glm::dot(position, normal)) {}

    DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
        const float cos_angle = glm::dot(ray.direction, m_normal);

        // enable backface culling at the same time
        if (cos_angle >= -std::numeric_limits<float>::epsilon()) {
            return false;
        }
        const float D = m_d - glm::dot(ray.origin, m_normal);
        const float t = D / cos_angle;

        // 5. Check if the intersection is behind the camera
        if (t < 0.0f) {
            return false;
        }
        if (out_intersection) {
            out_intersection->t = t;
            out_intersection->normal = m_normal;
            out_intersection->barycentric = { 0.0f, 0.0f };
            out_intersection->primitive = 0;
        }
        return true;
    }
    DOOB_NODISCARD AABB GetAABB() const override {
        return AABB{
            .min = { -INFINITY, -INFINITY, -INFINITY },
            .max = { INFINITY, INFINITY, INFINITY },
        };
    }

    glm::vec3 m_normal;
    float m_d;
};
} // namespace devs_out_of_bounds