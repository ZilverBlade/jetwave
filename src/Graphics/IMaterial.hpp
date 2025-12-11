#pragma once

#include <src/Core.hpp>
namespace devs_out_of_bounds {
struct MaterialInput {
    glm::vec3 position = {};
    glm::vec3 normal = {};
    glm::vec3 tangent = {};
    glm::vec2 uv = {};
};
struct MaterialOutput {
    glm::vec3 world_normal = {};
    glm::vec3 albedo_color = {};
    glm::vec3 specular_color = {};
    float specular_power = {};
    glm::vec3 emission_color = {};
};
struct IMaterial {
    IMaterial() = default;
    virtual ~IMaterial() = default;
    DOOB_NODISCARD virtual MaterialOutput Evaluate(const MaterialInput& input) const = 0;
};
} // namespace devs_out_of_bounds