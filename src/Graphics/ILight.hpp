#pragma once
#include <src/Core.hpp>
#include <src/Graphics/Ray.hpp>

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
struct ShadowingInput {
    const void* userdata = nullptr;
    // return true if visible
    FunctionPtr<bool(const Ray&, const void*)> fn_shadow_check = nullptr;
    uint32_t seed = 0;
};
struct LightOutput {
    glm::vec3 diffuse = {};
    glm::vec3 specular = {};
};
struct ILight {
    ILight() = default;
    virtual ~ILight() = default;
    DOOB_NODISCARD virtual LightOutput Evaluate(
        const LightInput& input, const ShadingInput& shading, const ShadowingInput& shadowing = {}) const = 0;
};
} // namespace devs_out_of_bounds