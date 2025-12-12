#pragma once
#include <src/Graphics/ILight.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
class DirectionalLight : public ILight {
public:
    DirectionalLight(const glm::vec3 direction, const glm::vec3& intensity, float src_angle_deg = 2.0f)
        : m_direction(glm::normalize(direction)), m_intensity(intensity),
          m_src_cos_angle(glm::cos(glm::radians(src_angle_deg))) {}

    DOOB_NODISCARD LightOutput Evaluate(const LightInput& input, const ShadingInput& shading, uint32_t& seed,
        const ShadowingInput& shadowing) const override {

        RandomStateAdvance(seed);

        glm::vec3 L = RandomCone(-m_direction, m_src_cos_angle, seed);

        if (shadowing.fn_shadow_check) {
            bool visibility = shadowing.fn_shadow_check(
                {
                    .origin = input.P,
                    .t_min = 0.001f,
                    .direction = L,
                },
                shadowing.userdata);
            if (!visibility) {
                return {};
            }
        }

        const glm::vec3 H = glm::normalize(L + input.V);
        const float NdH = glm::max(glm::dot(input.N, H), 0.0f);

        const float spec = glm::pow(NdH, shading.specular_power);

        glm::vec3 attenuated = glm::max(glm::dot(input.N, L), 0.0f) * m_intensity;
        return {
            .diffuse = attenuated,
            .specular = attenuated * spec,
        };
    }

    glm::vec3 m_direction;
    glm::vec3 m_intensity;
    float m_src_cos_angle = 1.0f;
};
} // namespace devs_out_of_bounds