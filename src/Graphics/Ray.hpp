#pragma once
#include <src/Core.hpp>
namespace devs_out_of_bounds {
struct Intersection {
    glm::vec3 position = {};
    float t = {};
    glm::vec2 barycentric = {};
    uint32_t primitive = 0;
    glm::vec3 flat_normal = {};
};
struct Ray {
    glm::vec3 origin = {};
    float t_min = 0.0f;
    glm::vec3 direction = {};
    float t_max = INFINITY;
};
} // namespace devs_out_of_bounds