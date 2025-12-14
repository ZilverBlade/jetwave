#pragma once
#include <src/Graphics/IBxDF.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
namespace bxdf {
    class DiffuseReflection : public IBxDF {
    public:
        DiffuseReflection(const glm::vec3& r, const glm::vec3& n) : m_r(r), m_normal(n) {}

        glm::vec3 Evaluate(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const override {
            return glm::abs(glm::dot(m_normal, wi)) * m_r * glm::one_over_pi<float>();
        }

        float Pdf(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const override {
            return glm::one_over_two_pi<float>();
        }

        glm::vec3 NextSample(const glm::vec3& wo, uint32_t& seed) const override {
            return RandomHemiAdv<UniformDistribution>(m_normal, seed);
        }

        BxDFType Type() const override { return BxDFType::DIFFUSE; }

    private:
        glm::vec3 m_r;
        glm::vec3 m_normal;
    };
} // namespace bxdf
} // namespace devs_out_of_bounds