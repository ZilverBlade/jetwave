#pragma once
#include <src/Core.hpp>
namespace devs_out_of_bounds {
struct Fragment {
    glm::vec3 position = {};
    glm::vec3 normal = {};
    glm::vec3 flat_normal = {};
    glm::vec3 tangent = {};
    glm::vec2 uv = {};
    bool b_front_face = false;
};
} // namespace devs_out_of_bounds