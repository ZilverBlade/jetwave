#pragma once
#include <src/Graphics/ILight.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
class AreaLight : public ILight {
public:
    AreaLight(const glm::vec3& center, const glm::vec2& extent, const glm::vec3& intensity)
        : m_center(center), m_extent(extent), m_intensity(intensity) {}

    DOOB_NODISCARD LightOutput Evaluate(const LightInput& input, const ShadingInput& shading, uint32_t& seed,
        const ShadowingInput& shadowing) const override {

        static constexpr glm::vec3 RIGHT = { 1, 0, 0 };
        static constexpr glm::vec3 FORWARD = { 0, 0, 1 };

        const float offx = m_extent.x * (2.0f * RandomFloatAdv(seed) - 1.0f);
        const float offy = m_extent.y * (2.0f * RandomFloatAdv(seed) - 1.0f);
        glm::vec3 light = m_center + RIGHT * offx + FORWARD * offy;

        const glm::vec3 fragToLight = light - input.P;
        float distSq = glm::dot(fragToLight, fragToLight);
        const float dist = glm::sqrt(distSq);
        glm::vec3 L = fragToLight / dist;

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

    glm::vec3 m_center;
    glm::vec3 m_normal;
    glm::vec2 m_extent;
    glm::vec3 m_intensity;
};
} // namespace devs_out_of_bounds