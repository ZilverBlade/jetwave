#pragma once

#include <src/Core.hpp>
namespace devs_out_of_bounds {
struct AABB {
    glm::vec3 min = {};
    glm::vec3 max = {};

    AABB Union(const AABB& other) {
        return {
            .min = glm::min(other.min, min),
            .max = glm::max(other.max, max),
        };
    }
};
} // namespace devs_out_of_bounds