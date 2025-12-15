#pragma once
#include <src/Graphics/BxDFs/GgxMicrofacetBrdf.hpp>
#include <src/Graphics/BxDFs/GgxMicrofacetBtdf.hpp>
#include <src/Graphics/BxDFs/PassthroughBtdf.hpp>
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
namespace material {
    class GlassMaterial : public IMaterial {
    public:
        void Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const override {
            if (out_bsdf) {
                if (m_ior == 1.0f) {
                    out_bsdf->Add<bxdf::PassthroughBtdf>(m_tint);
                } else {
                    out_bsdf->Add<bxdf::GgxMicrofacetBtdf>(m_tint, m_ior, m_roughness, input.normal);
                }
                out_bsdf->Add<bxdf::GgxMicrofacetBrdf>(glm::vec3(0.04f), m_roughness, input.normal);
            }
        }

        glm::vec3 m_tint = { 1, 1, 1 };
        float m_roughness = 0.02f;
        float m_ior = 1.5f;
    };
} // namespace material
} // namespace devs_out_of_bounds