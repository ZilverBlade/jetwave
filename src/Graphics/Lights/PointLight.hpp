#pragma once
#include <src/Graphics/ILight.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
class PointLight : public ILight {
public:
    PointLight(const glm::vec3& position, const glm::vec3& intensity)
        : m_position(position), m_intensity(intensity) {}

    LightSample Sample(const glm::vec3& P, uint32_t& seed) const override {
        glm::vec3 d = m_position - P;
        float distSq = glm::dot(d, d);
        float dist = std::sqrt(distSq);

        return { .L = d / dist,
            .Li = m_intensity * (1.0f / distSq), 
            .dist = dist };
    }

    glm::vec3 m_position;
    glm::vec3 m_intensity;
};
} // namespace devs_out_of_bounds