#pragma once
#include <src/Graphics/Mesh.hpp>
#include <vector>

namespace devs_out_of_bounds {
    namespace shape {
        class Trimesh : public IShape, public MeshInstance {
        public:
            Trimesh(const Mesh* mesh, const glm::mat4& transform, const std::vector<uint32_t>& indices)
                : MeshInstance(mesh, transform), m_indices(indices) {
            }

            DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
                bool b_hit = false;

                bool closest_b_backfacing = false;
                float closest_u, closest_v;
                glm::vec3 closest_edge1, closest_edge2;
                float closest_t = INFINITY;
                size_t closet_index = 0;
                for (size_t i = 0; i < m_indices.size(); i += 3) {
                    const glm::vec3 a = m_positions[m_indices[i + 0]];
                    const glm::vec3 b = m_positions[m_indices[i + 1]];
                    const glm::vec3 c = m_positions[m_indices[i + 2]];

                    const glm::vec3 edge1 = b - a;
                    const glm::vec3 edge2 = c - a;

                    const glm::vec3 pvec = glm::cross(ray.direction, edge2);

                    float det = glm::dot(edge1, pvec);

                    if (std::abs(det) < std::numeric_limits<float>::epsilon()) {
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
                        closet_index = i;
                    }

                    b_hit = true;
                }

                if (b_hit && out_intersection) {
                    out_intersection->t = closest_t;
                    out_intersection->position = ray.origin + ray.direction * closest_t;

                    out_intersection->barycentric = { closest_u, closest_v };
                    out_intersection->primitive = closet_index / 3;

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

            DOOB_NODISCARD AABB GetAABB() const override {
                glm::vec3 min = { INFINITY, INFINITY, INFINITY }, max = { -INFINITY, -INFINITY, -INFINITY };
                for (uint32_t i : m_indices) {
                    min = glm::min(m_positions[i], min);
                    max = glm::max(m_positions[i], max);
                }
                return AABB(min, max);
            }


        private:
            std::vector<uint32_t> m_indices;

            std::vector<uint32_t> m_indices;
        };
    } // namespace shape
} // namespace devs_out_of_bounds