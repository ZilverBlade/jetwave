#pragma once
#include <src/Graphics/IBxDF.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
namespace bxdf {
    class LambertBrdf : public IBxDF {
    public:
        LambertBrdf(const glm::vec3& r, const glm::vec3& n) : m_r(r), m_normal(n) {}

        glm::vec3 EvaluateCos(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const override {
            return glm::max(glm::dot(wi, m_normal), 0.0f) * m_r * glm::one_over_pi<float>();
        }

        float Pdf(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const override {
            return glm::max(glm::dot(wi, m_normal), 0.0f) * glm::one_over_pi<float>();
        }

        glm::vec3 NextSample(const glm::vec3& wo, uint32_t& seed) const override {
            return RandomCosWeightedHemiAdv<UniformDistribution>(m_normal, seed);
        }

        BxDFType Type() const override { return BxDFType::DIFFUSE; }

    private:
        glm::vec3 m_r;
        glm::vec3 m_normal;
    };
} // namespace bxdf
} // namespace devs_out_of_bounds