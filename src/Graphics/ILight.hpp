#pragma once
#include <glm/glm.hpp>
#include <src/Core.hpp>
namespace devs_out_of_bounds {
struct LightInput {
    glm::vec3 camera_position = {};
    glm::vec3 fragment_position = {};
    glm::vec3 fragment_normal = {};
    glm::vec3 fragment_normal = {};
    float specular_power = {};
};
struct LightOutput {
    glm::vec3 diffuse = {};
    glm::vec3 specular = {};
};
struct ILight {
    DOB_NODISCARD virtual LightOutput Evaluate(const LightInput& input) const = 0;
};
} // namespace devs_out_of_bounds