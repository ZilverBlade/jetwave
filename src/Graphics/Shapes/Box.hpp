#pragma once
#include <src/Graphics/IShape.hpp>

namespace devs_out_of_bounds {
namespace shape {
    class Box : public IShape {
    public:
        Box(const glm::vec3& center, const glm::vec3& extent)
            : m_min(center - extent / 2.0f), m_max(center + extent / 2.0f) {}

        DOOB_NODISCARD bool Intersect(const Ray& ray, Intersection* out_intersection) const override {
            const glm::vec3 inv_dir = 1.0f / (ray.direction + glm::sign(ray.direction) * 1e-9f);

            const glm::vec3 t_min_xyz = (m_min - ray.origin) * inv_dir;
            const glm::vec3 t_max_xyz = (m_max - ray.origin) * inv_dir;

            float tx_in = glm::min(t_min_xyz.x, t_max_xyz.x);
            float tx_out = glm::max(t_min_xyz.x, t_max_xyz.x);

            float ty_in = glm::min(t_min_xyz.y, t_max_xyz.y);
            float ty_out = glm::max(t_min_xyz.y, t_max_xyz.y);

            float tz_in = glm::min(t_min_xyz.z, t_max_xyz.z);
            float tz_out = glm::max(t_min_xyz.z, t_max_xyz.z);

            float t_in = glm::max(tx_in, ty_in, tz_in);
            float t_out = glm::min(tx_out, ty_out, tz_out);

            if (t_in > t_out) {
                return false;
            }

            float t_hit = t_in;
            bool b_front_facing = true;

            if (t_in < 0.0f) {
                t_hit = t_out;
                b_front_facing = false; // We are hitting the inside surface
                if (t_hit > ray.t_max)
                    return false; // Exit point too far
            }

            if (t_hit < ray.t_min)
                return false;

            if (out_intersection) {
                out_intersection->num_intersections = 1;
                const glm::vec3 hit_pos = ray.origin + ray.direction * t_hit;

                out_intersection->t = t_hit;
                out_intersection->position = hit_pos;
                out_intersection->primitive = 0; // or ID
                out_intersection->b_front_facing = b_front_facing;
                out_intersection->barycentric = { 0.0f, 0.0f };


                const glm::vec3 center = (m_min + m_max) * 0.5f;
                const glm::vec3 half_size = (m_max - m_min) * 0.5f;

                const glm::vec3 p = hit_pos - center;

                const glm::vec3 bias = glm::abs(p / (half_size + 1e-6f));

                out_intersection->flat_normal = glm::vec3(0.0f);

                if (bias.x > bias.y && bias.x > bias.z) {
                    out_intersection->flat_normal.x = glm::sign(p.x);
                } else if (bias.y > bias.z) {
                    out_intersection->flat_normal.y = glm::sign(p.y);
                } else {
                    out_intersection->flat_normal.z = glm::sign(p.z);
                }
            }
            return true;
        }
        DOOB_NODISCARD AABB GetAABB() const override {
            return AABB{
                .min = m_min,
                .max = m_max,
            };
        }
        DOOB_NODISCARD Fragment SampleFragment(const Intersection& intersection) const override {
            return {
                .position = intersection.position,
                .normal = intersection.flat_normal,
                .tangent = {},
                .uv = {},
            };
        }


        glm::vec3 m_min;
        glm::vec3 m_max;
    };
} // namespace shape
} // namespace devs_out_of_bounds