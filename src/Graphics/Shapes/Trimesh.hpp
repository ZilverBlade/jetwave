#pragma once
#include <src/Graphics/Mesh.hpp>
#include <vector>

namespace devs_out_of_bounds {
namespace shape {
    class Trimesh : public IShape {
    public:
        Trimesh(const MeshInstance* mesh_instance, const std::vector<uint32_t>& primitives) {
            m_primitive_ptr = new uint32_t[primitives.size()];
            m_primitive_count = static_cast<uint32_t>(primitives.size());
            for (size_t i = 0; i < primitives.size(); ++i) {
                m_primitive_ptr[i] = primitives[i];
            }
            m_attrib_ptr = mesh_instance->m_attributes.data();
            m_position_ptr = mesh_instance->m_positions.data();
            m_index_ptr = mesh_instance->m_index_ptr;
        }
        ~Trimesh() override { delete[] m_primitive_ptr; }

        DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
            bool b_hit = false;

            bool closest_b_backfacing = false;
            float closest_u, closest_v;
            glm::vec3 closest_edge1, closest_edge2;
            float closest_t = INFINITY;
            size_t closest_primitive = 0;
            uint32_t num_intersections = 0;

            for (uint32_t i = 0; i < m_primitive_count; ++i) {
                uint32_t p = m_primitive_ptr[i];

                const glm::vec3 a = m_position_ptr[m_index_ptr[p * 3 + 0]];
                const glm::vec3 b = m_position_ptr[m_index_ptr[p * 3 + 1]];
                const glm::vec3 c = m_position_ptr[m_index_ptr[p * 3 + 2]];

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
                ++num_intersections;
                if (t < closest_t) {
                    closest_t = t;
                    closest_b_backfacing = det < 0.0f;
                    closest_edge1 = edge1;
                    closest_edge2 = edge2;
                    closest_u = u;
                    closest_v = v;
                    closest_primitive = p;
                }

                b_hit = true;
            }

            if (b_hit && out_intersection) {
                out_intersection->num_intersections = num_intersections;
                out_intersection->t = closest_t;
                out_intersection->position = ray.origin + ray.direction * closest_t;

                out_intersection->barycentric = { closest_u, closest_v };
                out_intersection->primitive = closest_primitive;

                out_intersection->flat_normal = glm::normalize(glm::cross(closest_edge1, closest_edge2));
                out_intersection->b_front_facing = closest_b_backfacing ? 0 : 1;
                if (closest_b_backfacing) {
                    out_intersection->flat_normal = -out_intersection->flat_normal;
                }
            }
            return b_hit;
        }

        DOOB_NODISCARD Fragment SampleFragment(const Intersection& intersection) const override {
            float u = intersection.barycentric.x;
            float v = intersection.barycentric.y;
            float w = 1.0f - u - v;

            uint32_t i, j, k;
            i = m_index_ptr[intersection.primitive * 3 + 0];
            j = m_index_ptr[intersection.primitive * 3 + 1];
            k = m_index_ptr[intersection.primitive * 3 + 2];

            return {
                .position = u * m_position_ptr[i] + v * m_position_ptr[j] + w * m_position_ptr[k],
                .normal = u * m_attrib_ptr[i].normal + v * m_attrib_ptr[j].normal + w * m_attrib_ptr[k].normal,
                .tangent = u * m_attrib_ptr[i].tangent + v * m_attrib_ptr[j].tangent + w * m_attrib_ptr[k].tangent,
                .uv = u * m_attrib_ptr[i].uv + v * m_attrib_ptr[j].uv + w * m_attrib_ptr[k].uv,

                .b_front_face = intersection.b_front_facing != 0,
            };
        }

        DOOB_NODISCARD AABB GetAABB() const override {
            glm::vec3 min = { INFINITY, INFINITY, INFINITY }, max = { -INFINITY, -INFINITY, -INFINITY };
            for (uint32_t i = 0; i < m_primitive_count; ++i) {
                uint32_t idx = m_primitive_ptr[i];
                for (uint32_t j = 0; j < 3; ++j) {
                    uint32_t vertex_index = m_index_ptr[idx * 3 + j];
                    min = glm::min(m_position_ptr[vertex_index], min);
                    max = glm::max(m_position_ptr[vertex_index], max);
                }
            }
            return AABB(min, max);
        }


    private:
        const VertexAttributes* m_attrib_ptr;
        const glm::vec3* m_position_ptr;
        const uint32_t* m_index_ptr;

        uint32_t* m_primitive_ptr;
        uint32_t m_primitive_count;
    };
} // namespace shape
} // namespace devs_out_of_bounds