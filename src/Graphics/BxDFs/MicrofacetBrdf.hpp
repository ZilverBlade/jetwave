#pragma once
#include <src/Graphics/IBxDF.hpp>
#include <src/Graphics/Random.hpp>

namespace devs_out_of_bounds {
namespace bxdf {
    class MicrofacetBrdf : public IBxDF {
        float D_GGX(float dotNH) const {
            float a2 = m_alpha * m_alpha;
            float f = (dotNH * a2 - dotNH) * dotNH + 1.0f;
            return a2 / glm::max(glm::pi<float>() * f * f, 1e-12f);
        }

        float G1(const glm::vec3& v) const {
            float k = (m_alpha + 1.0f);
            k = (k * k) / 8.0f;
            float dotNV = glm::max(glm::dot(v, m_normal), 0.0f);
            return dotNV / glm::max((dotNV * (1.0f - k) + k), 1e-12f);
        }

        float G_Smith(const glm::vec3& wo, const glm::vec3& wi) const { return G1(wo) * G1(wi); }

        // --- 3. Fresnel (F) : Schlick ---
        glm::vec3 F_Schlick(float dotVH) const {
            const glm::vec3 f = (1.0f - m_f0) * (1.0f - dotVH);
            const glm::vec3 f2 = f * f;
            const glm::vec3 f5 = f2 * f2 * f;
            return m_f0 + f5;
        }

    public:
        MicrofacetBrdf(const glm::vec3& f0, float roughness, const glm::vec3& n)
            : m_f0(f0), m_alpha(glm::clamp(roughness * roughness, 1e-12f, 1.f)), m_normal(n) {}


        glm::vec3 Evaluate(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const override {
            float dotNL = glm::max(glm::dot(m_normal, wi), 0.0f);
            float dotNV = glm::max(glm::dot(m_normal, wo), 0.0f);

            if (dotNL == 0.0f || dotNV == 0.0f)
                return glm::vec3(0.0f);

            float dotNH = glm::max(glm::dot(m_normal, wm), 0.0f);
            float dotVH = glm::max(glm::dot(wo, wm), 0.0f);

            float D = D_GGX(dotNH);
            float G = G_Smith(wo, wi);
            glm::vec3 F = F_Schlick(dotVH);

            return (D * G * F) / std::max(4.0f * dotNV, 1e-12f);
        }

        float Pdf(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const override {
            float dotNH = glm::max(glm::dot(m_normal, wm), 0.0f);
            float dotVH = glm::max(glm::dot(wo, wm), 0.0f);

            // Probability of picking H
            float D = D_GGX(dotNH);
            float pdf_h = D * dotNH;

            // Jacobian transformation from H space to wi space
            return pdf_h / std::max(4.0f * dotVH, 1e-12f);
        }

        glm::vec3 NextSample(const glm::vec3& wo, uint32_t& seed) const override {
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

            return glm::reflect(-wo, H);
        }

        BxDFType Type() const override { return BxDFType::SPECULAR; }

    private:
        float m_alpha;      // Roughness^2
        glm::vec3 m_f0;     // Fresnel Colour
        glm::vec3 m_normal; // Normal
    };
} // namespace bxdf
} // namespace devs_out_of_bounds