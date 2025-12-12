#pragma once
#include <src/Graphics/ILight.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
class DirectionalLight : public ILight {
public:
    DirectionalLight(const glm::vec3 direction, const glm::vec3& intensity, float src_angle_deg = 2.0f)
        : m_direction(glm::normalize(direction)), m_intensity(intensity),
          m_src_cos_angle(glm::cos(glm::radians(src_angle_deg))) {}

    LightSample Sample(const glm::vec3& P, uint32_t& seed) const override {
        glm::vec3 L = RandomCone(-m_direction, m_src_cos_angle, seed);

        return {
            .L = L,
            .Li = m_intensity, 
            .dist = INFINITY,
        };
    }

    glm::vec3 m_direction;
    glm::vec3 m_intensity;
    float m_src_cos_angle = 1.0f;
};
} // namespace devs_out_of_bounds