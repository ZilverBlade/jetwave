#pragma once
#include <src/Core.hpp>
namespace devs_out_of_bounds {
struct Intersection {
    glm::vec2 barycentric = {};
    float t = {};
    uint32_t primitive = 0;
    glm::vec3 normal = {};
    uint32_t _padding;
};
struct Ray {
    glm::vec3 origin = {};
    float t_min = 0.0f;
    glm::vec3 direction = {};
    float t_max = INFINITY;
};
} // namespace devs_out_of_bounds