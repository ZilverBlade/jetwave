#pragma once

#include <src/Core.hpp>
#include <src/Graphics/Fragment.hpp>

namespace devs_out_of_bounds {
struct MaterialOutput {
    glm::vec3 world_normal = {};
    glm::vec3 albedo_color = {};
    glm::vec3 specular_color = {};
    float specular_power = {};
    glm::vec3 emission_color = {};
    float opacity = 1.0f;
};
struct IMaterial {
    IMaterial() = default;
    virtual ~IMaterial() = default;
    DOOB_NODISCARD virtual MaterialOutput Evaluate(const Fragment& input) const = 0;
    DOOB_NODISCARD virtual bool EvaluateDiscard(const Fragment& input) const = 0;
};
} // namespace devs_out_of_bounds