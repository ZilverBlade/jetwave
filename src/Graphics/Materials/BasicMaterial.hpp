#pragma once
#include <src/Graphics/BxDFs/LambertBrdf.hpp>
#include <src/Graphics/BxDFs/GgxMicrofacetBrdf.hpp>
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
namespace material {
    class BasicMaterial : public IMaterial {
    public:
        bool Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const {
            if (out_bsdf) {
                out_bsdf->Add<bxdf::LambertBrdf>(m_albedo, input.normal);
                out_bsdf->Add<bxdf::GgxMicrofacetBrdf>(glm::vec3(0.04f * m_specular), m_roughness, input.normal);
            }
            return true;
        }
        bool IsOpaque() const override { return true; }

        glm::vec3 m_albedo = { 1, 1, 1 };
        float m_roughness = 0.5f;
        float m_specular = 1.0f;
    };
} // namespace material
} // namespace devs_out_of_bounds