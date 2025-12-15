#pragma once
#include <src/Graphics/BxDFs/Common/Microfacet.hpp>
#include <src/Graphics/IBxDF.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
namespace bxdf {
    class PassthroughBtdf : public IBxDF {
    public:
        static constexpr float DIRAC_EPSILON = 1.0f - 1e-6f;

        PassthroughBtdf(const glm::vec3& transmissionColor, float opacity = 0.0f)
            : m_t(transmissionColor), m_opacity(opacity) {}

        glm::vec3 Evaluate(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const override {
            if (glm::dot(wi, -wo) > DIRAC_EPSILON) {
                return m_t * (1.0f - m_opacity);
            }
            return glm::vec3(0.0f);
        }

        glm::vec3 NextSample(const glm::vec3& wo, uint32_t& seed) const override {
            float o = RandomFloatAdv<UniformDistribution>(seed);
            if (o < m_opacity) {
                return glm::vec3(0.0f); // Absorb the ray
            }
            return -wo;
        }

        float Pdf(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const override {
            return glm::step(DIRAC_EPSILON, glm::dot(wi, -wo)) * (1.0f - m_opacity);
        }
        BxDFType Type() const override { return BxDFType::TRANSMISSION; }

    private:
        glm::vec3 m_t;
        float m_opacity;
    };
} // namespace bxdf
} // namespace devs_out_of_bounds