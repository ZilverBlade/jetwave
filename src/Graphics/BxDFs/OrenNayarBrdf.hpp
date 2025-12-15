#pragma once
#include <src/Graphics/IBxDF.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
namespace bxdf {
    class OrenNayarBrdf : public IBxDF {
    public:
        OrenNayarBrdf(const glm::vec3& r, float sigma, const glm::vec3& n) : m_r(r), m_sigma(sigma), m_normal(n) {}

        float GetPhi(const glm::vec3& v, const glm::vec3& T, const glm::vec3& B) const {
            return glm::atan(glm::dot(v, B), glm::dot(v, T));
        }


        glm::vec3 Evaluate(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const override {
            float cosThetaI = glm::dot(m_normal, wi);
            float cosThetaO = glm::dot(m_normal, wo);
            if (cosThetaI <= 0.0f || cosThetaO <= 0.0f) {
                return glm::vec3(0.0f);
            }

            glm::vec3 T, B;
            CreateTangentSpace(T, B, m_normal);

            float phiI = GetPhi(wi, T, B);
            float phiO = GetPhi(wo, T, B);

            float cosPhiIminO = glm::cos(phiI - phiO);

            float acosThetaI = glm::acos(cosThetaI);
            float acosThetaO = glm::acos(cosThetaO);

            float s2 = m_sigma * m_sigma;
            float alpha = glm::max(acosThetaI, acosThetaO);
            float beta = glm::min(acosThetaI, acosThetaO);

            float C1 = 1.0f - 0.5f * s2 / (s2 + 0.33f);
            float C2;
            if (cosPhiIminO >= 0.0f) {
                C2 = 0.45f * s2 / (s2 + 0.09f) * glm::sin(alpha);
            } else {
                C2 = 0.45f * s2 / (s2 + 0.09f) * (glm::sin(alpha) - glm::pow(2.0f * beta / glm::pi<float>(), 3));
            }
            float C3 =
                0.125f * (s2 / (s2 + 0.09f)) * glm::pow(4.0f * alpha * beta / (glm::pi<float>() * glm::pi<float>()), 2);

            float L1 = C1 + C2 * cosPhiIminO * glm::tan(beta) +
                       C3 * (1.f - glm::abs(cosPhiIminO)) * glm::tan((alpha + beta) / 2.0f);
            float L2 = 0.17f * s2 / (s2 + 0.13f) * (1.0f - cosPhiIminO * glm::pow(2.f * beta / glm::pi<float>(), 2));
            return glm::one_over_pi<float>() * cosThetaI * m_r * (L1 + m_r * L2);
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
        float m_sigma;
    };
} // namespace bxdf
} // namespace devs_out_of_bounds