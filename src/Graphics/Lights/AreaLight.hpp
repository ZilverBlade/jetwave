#pragma once
#include <src/Graphics/ILight.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
namespace light {
    class AreaLight : public ILight {
    public:
        AreaLight(const glm::vec3& center, const glm::vec2& extent, const glm::vec3& candelas)
            : m_center(center), m_extent(extent), m_cd(candelas) {}

        LightSample Sample(const glm::vec3& P, uint32_t& seed) const override {
            float offx = m_extent.x * (2.0f * RandomFloatAdv<UniformDistribution>(seed) - 1.0f);
            float offy = m_extent.y * (2.0f * RandomFloatAdv<UniformDistribution>(seed) - 1.0f);

            glm::vec3 lightPos = m_center + glm::vec3(offx, 0, offy);

            glm::vec3 d = lightPos - P;
            float distSq = glm::dot(d, d);
            float dist = std::sqrt(distSq);
            glm::vec3 L = d / dist;

            float cos_light = glm::max(glm::dot(m_normal, -L), 0.0f);
            float area = (m_extent.x * 2.0f) * (m_extent.y * 2.0f);

            return { .L = L, .Li = m_cd * (cos_light * area / distSq), .dist = dist };
        }

        glm::vec3 m_center;
        glm::vec3 m_normal = { 0, -1, 0 };
        glm::vec2 m_extent;
        glm::vec3 m_cd;
    };
} // namespace light
} // namespace devs_out_of_bounds