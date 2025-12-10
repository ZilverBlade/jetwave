#pragma once
#include <glm/glm.hpp>
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
    glm::vec3 direction = {};
};
} // namespace devs_out_of_bounds