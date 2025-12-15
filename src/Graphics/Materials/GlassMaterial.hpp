#pragma once
#include <src/Graphics/BxDFs/GgxMicrofacetBrdf.hpp>
#include <src/Graphics/BxDFs/GgxMicrofacetBtdf.hpp>
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
namespace material {
    class GlassMaterial : public IMaterial {
    public:
        bool Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const {
            if (out_bsdf) {
                out_bsdf->Add<bxdf::GgxMicrofacetBtdf>(m_tint, m_roughness, m_ior, input.normal);
                out_bsdf->Add<bxdf::GgxMicrofacetBrdf>(glm::vec3(0.04f), m_roughness, input.normal);
            }
            return true;
        }
        bool IsOpaque() const override { return true; }

        glm::vec3 m_tint = { 1, 1, 1 };
        float m_roughness = 0.0f;
        float m_ior = 1.5f;
    };
} // namespace material
} // namespace devs_out_of_bounds