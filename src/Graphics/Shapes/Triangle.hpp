#pragma once
#include <src/Graphics/IShape.hpp>
#include <vector>

namespace devs_out_of_bounds {
// Triangles are counter clockwise!!
class Triangle : public IShape {
public:
    Triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) : m_a(a), m_b(b), m_c(c) {}

    DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
        const glm::vec3 ab = m_b - m_a;
        const glm::vec3 ac = m_c - m_a;

        const glm::vec3 n = glm::normalize(glm::cross(ab, ac));

        const float cos_angle = glm::dot(ray.direction, n);
        if (cos_angle >= -std::numeric_limits<float>::epsilon()) {
            return false;
        }
        const float t = (glm::dot(n, m_a) - glm::dot(ray.origin, n)) / cos_angle;
        const glm::vec3 p = ray.origin + t * ray.direction;

        // a + Beta * ab + Gamma * ac = P
        const glm::vec3 ap = p - m_a;

        float beta = glm::dot(ab, ap) / glm::dot(ab, ab);
        float gamma = glm::dot(ac, ap) / glm::dot(ac, ab);

        if ((beta + gamma) > 1.0f || beta < 0.0f || gamma < 0.0f) {
            return false;
        }

        if (out_intersection) {
            out_intersection->position = p;
            out_intersection->t = t;
            out_intersection->barycentric = { beta, gamma };
            out_intersection->primitive = 0;
            out_intersection->normal = n;
        }

        return true;
    }

    DOOB_NODISCARD AABB GetAABB() const override { return AABB(m_a, m_a).Union({ m_b, m_b }).Union({ m_c, m_c }); }

    glm::vec3 m_a, m_b, m_c;
};
} // namespace devs_out_of_bounds