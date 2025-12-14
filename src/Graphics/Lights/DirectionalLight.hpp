#pragma once
#include <src/Graphics/ILight.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
namespace light {
    class DirectionalLight : public ILight {
    public:
        DirectionalLight(const glm::vec3 direction, const glm::vec3& candelas, float src_angle_deg = 2.0f)
            : m_direction(glm::normalize(direction)), m_cd(candelas),
              m_src_cos_angle(glm::cos(glm::radians(src_angle_deg))) {}

        LightSample Sample(const glm::vec3& P, uint32_t& seed) const override {
            glm::vec3 L = RandomConeAdv<UniformDistribution>(-m_direction, m_src_cos_angle, seed);

            return {
                .L = L,
                .Li = m_cd,
                .dist = INFINITY,
            };
        }

        glm::vec3 m_direction;
        glm::vec3 m_cd;
        float m_src_cos_angle = 1.0f;
    };
} // namespace light
} // namespace devs_out_of_bounds