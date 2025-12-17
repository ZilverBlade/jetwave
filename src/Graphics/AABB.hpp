#pragma once

#include <src/Core.hpp>
#include <src/Graphics/Ray.hpp>

namespace devs_out_of_bounds {
struct AABB {
    glm::vec3 min = {};
    glm::vec3 max = {};

    DOOB_NODISCARD AABB Union(const AABB& other) {
        return {
            .min = glm::min(other.min, min),
            .max = glm::max(other.max, max),
        };
    }

    DOOB_NODISCARD bool RayIntersects(const Ray& ray, float* out_in_t, float* out_out_t) const {
        const glm::vec3 inv_dir = 1.0f / (ray.direction + glm::sign(ray.direction) * 1e-9f);

        const glm::vec3 t_min_xyz = (min - ray.origin) * inv_dir;
        const glm::vec3 t_max_xyz = (max - ray.origin) * inv_dir;

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
        if (out_out_t) {
            *out_out_t = std::max(t_out, t_in);
        }
        if (out_in_t) {
            *out_in_t = std::min(t_out, t_in);
        }
    }
};
} // namespace devs_out_of_bounds