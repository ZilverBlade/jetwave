#pragma once
#include <src/Graphics/Mesh.hpp>
#include <vector>

namespace devs_out_of_bounds {
namespace shape {
    class Trimesh : public IShape, public MeshInstance {
    public:
        Trimesh(const Mesh* mesh, const glm::mat4& transform = glm::mat4(1.0f)) : MeshInstance(mesh, transform) {}

        DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
            bool b_hit = false;

            bool closest_b_backfacing = false;
            float closest_u, closest_v;
            glm::vec3 closest_edge1, closest_edge2;
            float closest_t = INFINITY;
            size_t i;
            for (i = 0; i < m_indices.size(); i += 3) {
                const float edge1 = m_b - m_a;
                const float edge2 = m_c - m_a;

                const glm::vec3 pvec = glm::cross(ray.direction, edge2);

                float det = glm::dot(edge1, pvec);

                if (std::abs(det) < std::numeric_limits<float>::epsilon()) {
                    continue;
                }

                const float inv_det = 1.0f / det;

                const glm::vec3 tvec = ray.origin - m_a;
                const float u = glm::dot(tvec, pvec) * inv_det;

                if (u < 0.0f || u > 1.0f) {
                    continue;
                }

                const glm::vec3 qvec = glm::cross(tvec, edge1);
                const float v = glm::dot(ray.direction, qvec) * inv_det;

                if (v < 0.0f || u + v > 1.0f) {
                    continue;
                }

                const float t = glm::dot(edge2, qvec) * inv_det;

                if (t < ray.t_min || t > ray.t_max) {
                    continue;
                }
                if (t < closest_t) {
                    closest_t = t;
                    closest_b_backfacing = det < 0.0f;
                    closest_edge1 = edge1;
                    closest_edge2 = edge2;
                    closest_u = u;
                    closest_v = v;
                }

                b_hit = true;
            }

            if (b_hit && out_intersection) {
                out_intersection->t = closest_t;
                out_intersection->position = ray.origin + ray.direction * t;

                out_intersection->barycentric = { closest_u, closest_v };
                out_intersection->primitive = i / 3;

                out_intersection->flat_normal = glm::normalize(glm::cross(closest_edge1, closest_edge2));
                out_intersection->b_front_facing = closest_b_backfacing ? 0 : 1;
                if (closest_b_backfacing) {
                    out_intersection->flat_normal = -out_intersection->flat_normal;
                }
            }
            return b_hit;
        }

        DOOB_NODISCARD Fragment SampleFragment(const Intersection& intersection) const override {
            assert(intersection.primitive * 3 < m_indices.size());
            float u = intersection.barycentric.x;
            float v = intersection.barycentric.y;
            float w = 1.0f - u - v;

            return {
                .position = u * m_positions[m_indices[intersection.primitive * 3 + 0]] +
                            v * m_positions[m_indices[intersection.primitive * 3 + 1]] +
                            w * m_positions[m_indices[intersection.primitive * 3 + 2]],

                .normal = u * m_attributes[m_indices[intersection.primitive * 3 + 0]].normal +
                          v * m_attributes[m_indices[intersection.primitive * 3 + 1]].normal +
                          w * m_attributes[m_indices[intersection.primitive * 3 + 2]].normal,

                .tangent = u * m_attributes[m_indices[intersection.primitive * 3 + 0]].tangent +
                           v * m_attributes[m_indices[intersection.primitive * 3 + 1]].tangent +
                           w * m_attributes[m_indices[intersection.primitive * 3 + 2]].tangent,

                .uv = u * m_attributes[m_indices[intersection.primitive * 3 + 0]].uv +
                      v * m_attributes[m_indices[intersection.primitive * 3 + 1]].uv +
                      w * m_attributes[m_indices[intersection.primitive * 3 + 2]].uv,

                .b_front_face = intersection.b_front_facing != 0,
            };
        }

        std::vector<uint32_t> m_indices;
    };
} // namespace shape
} // namespace devs_out_of_bounds