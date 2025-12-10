#pragma once
#include <src/Graphics/ILight.hpp>

namespace devs_out_of_bounds {
class PointLight : public ILight {
public:
    PointLight(const glm::vec3& position, const glm::vec3& intensity) : m_position(position), m_intensity(intensity) {}

    DOOB_NODISCARD LightOutput Evaluate(
        const LightInput& input, const ShadingInput& shading, const ShadowingInput& shadowing) const override {
        const glm::vec3 fragToLight = m_position - input.P;
        const float distSq = glm::dot(fragToLight, fragToLight);
        const float dist = glm::sqrt(distSq);
        const glm::vec3 L = fragToLight / dist;

        if (shadowing.fn_shadow_check) {
            bool visibility = shadowing.fn_shadow_check(
                {
                    .origin = input.P,
                    .t_min = 0.001f,
                    .direction = L,
                    .t_max = dist,
                },
                shadowing.userdata);
            if (!visibility) {
                return {};
            }
        }

        const float attenuation = 1.0f / distSq;

        const glm::vec3 H = glm::normalize(L + input.V);
        const float NdH = glm::max(glm::dot(input.N, H), 0.0f);

        const float spec = glm::pow(NdH, shading.specular_power);

        glm::vec3 attenuated = attenuation * m_intensity;
        return {
            .diffuse = attenuated,
            .specular = attenuated * spec,
        };
    }

    glm::vec3 m_position;
    glm::vec3 m_intensity;
};
} // namespace devs_out_of_bounds