#pragma once
#include <src/Graphics/Mesh.hpp>
#include <vector>

namespace devs_out_of_bounds {
namespace shape {
    class Trimesh : NoCopy {
    public:
        Trimesh(const MeshInstance* mesh_instance, const std::vector<uint32_t>& primitives) {
            m_primitive_ptr = new uint32_t[primitives.size()];
            m_primitive_count = static_cast<uint32_t>(primitives.size());
            for (size_t i = 0; i < primitives.size(); ++i) {
                m_primitive_ptr[i] = primitives[i];
            }
            m_position_ptr = mesh_instance->m_positions.data();
            m_index_ptr = mesh_instance->m_index_ptr;
        }
        ~Trimesh() { delete[] m_primitive_ptr; }

        Trimesh(Trimesh&& other) noexcept {
            m_primitive_ptr = other.m_primitive_ptr;
            m_primitive_count = other.m_primitive_count;
            m_position_ptr = other.m_position_ptr;
            m_index_ptr = other.m_index_ptr;
            other.m_primitive_ptr = nullptr;
            other.m_primitive_count = 0;
            other.m_position_ptr = nullptr;
            other.m_index_ptr = nullptr;
        }
        Trimesh& operator=(Trimesh&& other) noexcept {
            if (this != &other) {
                if (m_primitive_ptr) {
                    delete[] m_primitive_ptr;
                }
                m_primitive_ptr = other.m_primitive_ptr;
                m_primitive_count = other.m_primitive_count;
                m_position_ptr = other.m_position_ptr;
                m_index_ptr = other.m_index_ptr;
                other.m_primitive_ptr = nullptr;
                other.m_primitive_count = 0;
                other.m_position_ptr = nullptr;
                other.m_index_ptr = nullptr;
            }
            return *this;
        }

        DOOB_NODISCARD DOOB_FORCEINLINE bool Intersect(const Ray& ray, Intersection* out_intersection) const {
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

    private:
        const glm::vec3* m_position_ptr = nullptr;
        const uint32_t* m_index_ptr = nullptr;

        uint32_t* m_primitive_ptr = nullptr;
        uint32_t m_primitive_count = 0;
    };
} // namespace shape
} // namespace devs_out_of_bounds