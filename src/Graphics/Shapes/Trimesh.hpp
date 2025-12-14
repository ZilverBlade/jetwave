#pragma once
#include <src/Graphics/Mesh.hpp>
#include <vector>

namespace devs_out_of_bounds {
namespace shape {
    class Trimesh : public IShape {
    public:
        Trimesh(Mesh* trimesh, const std::vector<uint32_t>& indices) : m_trimesh(trimesh), m_indices(indices) {
            assert((m_indices.size() % 3) == 0 && "Indices must be a multiple of 3!");
        }

        DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
            auto& positions = m_trimesh->m_positions;
            for (size_t i = 0; i < m_indices.size(); i += 3) {
                glm::vec3 a = positions[i];
                glm::vec3 b = positions[i + 1];
                glm::vec3 c = positions[i + 2];

                const glm::vec3 edge1 = b - a;
                const glm::vec3 edge2 = c - a;

                const glm::vec3 pvec = glm::cross(ray.direction, edge2);

                const float det = glm::dot(edge1, pvec);

                if (det < std::numeric_limits<float>::epsilon()) {
                    continue;
                }

                const float inv_det = 1.0f / det;

                const glm::vec3 tvec = ray.origin - a;
                const float u = glm::dot(tvec, pvec) * inv_det;

                if (u < 0.0f || u > 1.0f) {
                    continue;
                }

                const glm::vec3 qvec = glm::cross(tvec, edge1);
                const float v = glm::dot(ray.direction, qvec) * inv_det;

                if (v < 0.0f || u + v > 1.0f) {
                    return false;
                }

                const float t = glm::dot(edge2, qvec) * inv_det;

                if (t < ray.t_min || t > ray.t_max) {
                    continue;
                }

                if (out_intersection) {
                    out_intersection->t = t;
                    out_intersection->position = ray.origin + ray.direction * t;

                    out_intersection->barycentric = { u, v };
                    out_intersection->primitive = i / 3;

                    out_intersection->flat_normal = glm::normalize(glm::cross(edge1, edge2));
                }
                return true;
            }
            return false;
        }

        DOOB_NODISCARD Fragment SampleFragment(const Intersection& intersection) const override {
            return {
                .position = intersection.position,
                .normal = intersection.flat_normal,
                .tangent = {},
                .uv = {},
            };
        }

        Trimesh* m_trimesh;
        std::vector<uint32_t> m_indices;
    };
} // namespace shape
} // namespace devs_out_of_bounds