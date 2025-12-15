#pragma once
#include <src/Graphics/BxDFs/GgxMicrofacetBrdf.hpp>
#include <src/Graphics/BxDFs/LambertBrdf.hpp>
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
namespace material {
    class ClearcoatMaterial : public IMaterial {
    public:
        void Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const override {
            if (out_bsdf) {
                glm::vec3 nor = glm::normalize(input.normal);
                out_bsdf->Add<bxdf::LambertBrdf>(m_albedo, nor);
                out_bsdf->Add<bxdf::GgxMicrofacetBrdf>(glm::vec3(0.04f), m_roughness, nor);
                out_bsdf->Add<bxdf::GgxMicrofacetBrdf>(glm::vec3(0.04f * m_clearcoat), m_clearcoat_roughness, nor);
            }
        }

        glm::vec3 m_albedo = { 1, 1, 1 };
        float m_roughness = 0.5f;
        float m_clearcoat = 1.0f;
        float m_clearcoat_roughness = 0.02f;
    };
} // namespace material
} // namespace devs_out_of_bounds