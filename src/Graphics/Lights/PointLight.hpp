#pragma once
#include <src/Graphics/ILight.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
namespace light {
    class PointLight : public ILight {
    public:
        PointLight(const glm::vec3& position, const glm::vec3& candelas) : m_position(position), m_cd(candelas) {}

        LightSample Sample(const glm::vec3& P, uint32_t& seed) const override {
            glm::vec3 d = m_position - P;
            float distSq = glm::dot(d, d);
            float dist = std::sqrt(distSq);

            return { .L = d / dist, .Li = m_cd * (1.0f / distSq), .dist = dist };
        }

        glm::vec3 m_position;
        glm::vec3 m_cd;
    };
} // namespace light
} // namespace devs_out_of_bounds