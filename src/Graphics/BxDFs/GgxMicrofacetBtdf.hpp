#pragma once
#include <src/Graphics/BxDFs/Common/Microfacet.hpp>
#include <src/Graphics/IBxDF.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
namespace bxdf {
    class GgxMicrofacetBtdf : public IBxDF {
    public:
        GgxMicrofacetBtdf(const glm::vec3& transmissionColor, float ior, float roughness, const glm::vec3& n)
            : m_t(transmissionColor), m_normal(n), m_eta(ior), m_alpha(std::max(1e-3f, roughness * roughness)) {}

        glm::vec3 EvaluateCos(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const override {
            if (SameHemisphere(wo, wi, m_normal))
                return glm::vec3(0.0f); // Refraction must cross the surface

            float cosThetaO = glm::dot(wo, m_normal);
            float cosThetaI = glm::dot(wi, m_normal);

            bool b_entering = cosThetaO > 0.0f;
            float eta_wo = b_entering ? 1.0f : m_eta; // IOR of wo's side
            float eta_wi = b_entering ? m_eta : 1.0f; // IOR of wi's side

            glm::vec3 Hx = eta_wo * wo + eta_wi * wi;
            if (glm::dot(Hx, Hx) < 1e-12f)
                return glm::vec3(0.0f);
            glm::vec3 H = -glm::normalize(Hx);

            float dotNH = std::abs(glm::dot(m_normal, H));
            float dotVH = std::abs(glm::dot(wo, H));
            float dotLH = std::abs(glm::dot(wi, H));

            float D = D_GGX(dotNH, m_alpha * m_alpha);
            float G = G_Smith(std::abs(cosThetaO), std::abs(cosThetaI), m_alpha);

            float sqrt_f0 = (eta_wi - eta_wo) / std::max(eta_wi + eta_wo, 1e-12f);
            glm::vec3 F = F_Schlick(dotVH, glm::vec3(sqrt_f0 * sqrt_f0));

            float sqrtDenom = (eta_wo * glm::dot(wo, H) + eta_wi * glm::dot(wi, H));

            // Walter '07 Eq. 21
            float value = (dotLH * dotVH * eta_wi * eta_wi * D * G) /
                          std::max(std::abs(cosThetaI) * std::abs(cosThetaO) * sqrtDenom * sqrtDenom, 1e-12f);

            return m_t * value * (1.0f - F);
        }

        // 2. SAMPLE: Generate a new ray direction
        glm::vec3 NextSample(const glm::vec3& wo, uint32_t& seed) const override {
            // A. Flip normal if we are inside the object
            bool entering = glm::dot(wo, m_normal) > 0.0f;
            glm::vec3 N = entering ? m_normal : -m_normal;
            float eta = entering ? (1.0f / m_eta) : m_eta;

            glm::vec2 u = { RandomFloatAdv<UniformDistribution>(seed), RandomFloatAdv<UniformDistribution>(seed) };

            float a2 = m_alpha * m_alpha;
            float phi = 2.0f * glm::pi<float>() * u.x;
            float cosTheta = std::sqrt((1.0f - u.y) / (1.0f + (a2 - 1.0f) * u.y));
            float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);

            glm::vec3 H_local(sinTheta * std::cos(phi),
                cosTheta, // Y is up in local space usually, or Z depending on your convention
                sinTheta * std::sin(phi));

            glm::vec3 up = std::abs(m_normal.z) < 0.999f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
            glm::vec3 tangent = glm::normalize(glm::cross(up, m_normal));
            glm::vec3 bitangent = glm::cross(m_normal, tangent);

            glm::vec3 H = glm::normalize(tangent * H_local.x + m_normal * H_local.y + bitangent * H_local.z);

            // Refract the view vector (wo) through the microfacet (H)
            glm::vec3 wi = glm::refract(-wo, H, eta);

            // If refract returns 0 length, light is trapped. Return 0 (absorb)
            if (glm::length(wi) < 0.01f)
                return glm::vec3(0.0f);

            return glm::normalize(wi);
        }
        float Pdf(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const override {
            if (SameHemisphere(wo, wi, m_normal))
                return 0.0f;

            bool b_entering = glm::dot(wo, m_normal) > 0.0f;
            float eta_wo = b_entering ? 1.0f : m_eta;
            float eta_wi = b_entering ? m_eta : 1.0f;

            glm::vec3 H = -glm::normalize(eta_wo * wo + eta_wi * wi);

            float dotNH = glm::max(glm::dot(m_normal, H), 0.0f);
            float D = D_GGX(dotNH, m_alpha * m_alpha);
            float pdf_h = D * dotNH;

            // Jacobian
            float sqrtDenom = (eta_wo * glm::dot(wo, H) + eta_wi * glm::dot(wi, H));
            float dwh_dwi = (eta_wi * eta_wi * std::abs(glm::dot(wi, H))) / (sqrtDenom * sqrtDenom);

            return pdf_h * dwh_dwi;
        }

        BxDFType Type() const override { return BxDFType::TRANSMISSION; }

    private:
        glm::vec3 m_t;
        glm::vec3 m_normal;
        float m_alpha;
        float m_eta;

        // Helper: Check if vectors are on same side of plane
        bool SameHemisphere(const glm::vec3& a, const glm::vec3& b, const glm::vec3& n) const {
            return (glm::dot(a, n) * glm::dot(b, n)) > 0.0f;
        }
    };
} // namespace bxdf
} // namespace devs_out_of_bounds