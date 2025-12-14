#pragma once
#include <src/Graphics/IShape.hpp>
#include <vector>

namespace devs_out_of_bounds {
namespace shape {
    // Triangles are counter clockwise!!
    class Triangle : public IShape {
    public:
        Triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) : m_a(a), m_b(b), m_c(c) {}

        DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
            const glm::vec3 edge1 = m_b - m_a;
            const glm::vec3 edge2 = m_c - m_a;

            const glm::vec3 pvec = glm::cross(ray.direction, edge2);

            const float det = glm::dot(edge1, pvec);

            if (std::abs(det) < std::numeric_limits<float>::epsilon()) {
                return false;
            }

            bool b_backfacing = det < 0.0f;

            const float inv_det = 1.0f / det;

            const glm::vec3 tvec = ray.origin - m_a;
            const float u = glm::dot(tvec, pvec) * inv_det;

            if (u < 0.0f || u > 1.0f) {
                return false;
            }

            const glm::vec3 qvec = glm::cross(tvec, edge1);
            const float v = glm::dot(ray.direction, qvec) * inv_det;

            if (v < 0.0f || u + v > 1.0f) {
                return false;
            }

            const float t = glm::dot(edge2, qvec) * inv_det;

            if (t < ray.t_min || t > ray.t_max) {
                return false;
            }

            if (out_intersection) {
                out_intersection->t = t;
                out_intersection->position = ray.origin + ray.direction * t;

                out_intersection->barycentric = { u, v };
                out_intersection->primitive = 0;

                out_intersection->flat_normal = glm::normalize(glm::cross(edge1, edge2));
                out_intersection->b_front_facing = b_backfacing ? 0 : 1;
                if (b_backfacing) {
                    out_intersection->flat_normal = -out_intersection->flat_normal;
                }
            }
        }

        DOOB_NODISCARD AABB GetAABB() const override { return AABB(m_a, m_a).Union({ m_b, m_b }).Union({ m_c, m_c }); }

        DOOB_NODISCARD Fragment SampleFragment(const Intersection& intersection) const override {
            return {
                .position = intersection.position,
                .normal = intersection.flat_normal,
                .tangent = {},
                .uv = {},
            };
        }

        glm::vec3 m_a, m_b, m_c;
    };
} // namespace shape
} // namespace devs_out_of_bounds