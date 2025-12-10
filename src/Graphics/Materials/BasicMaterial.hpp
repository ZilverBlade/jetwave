#pragma once
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
class BasicMaterial : public IMaterial {
public:
    BasicMaterial(const glm::vec3& albedo_color, const glm::vec3& specular_color, float specular_power)
        : m_albedo_color(albedo_color), m_specular_color(specular_color), m_specular_power(specular_power) {}

    DOOB_NODISCARD MaterialOutput Evaluate(const MaterialInput& input) const override {
        return {
            .albedo_color = m_albedo_color,
            .specular_color = m_specular_color,
            .specular_power = m_specular_power,
        };
    }
    glm::vec3 m_albedo_color;
    glm::vec3 m_specular_color;
    float m_specular_power;
};
} // namespace devs_out_of_bounds