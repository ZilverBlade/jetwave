#pragma once
#include <glm/glm.hpp>
#include <src/Core.hpp>
namespace devs_out_of_bounds {
struct LightInput {
    glm::vec3 eye = {};
    glm::vec3 P = {};
    glm::vec3 V = {};
    glm::vec3 N = {};
};
struct ShadingInput {
    float specular_power = {};
};
struct LightOutput {
    glm::vec3 diffuse = {};
    glm::vec3 specular = {};
};
struct ILight {
    ILight() = default;
    virtual ~ILight() = default;
    DOOB_NODISCARD virtual LightOutput Evaluate(const LightInput& input, const ShadingInput& shading) const = 0;
};
} // namespace devs_out_of_bounds