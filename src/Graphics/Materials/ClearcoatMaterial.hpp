#pragma once
#include <src/Graphics/BxDFs/LambertReflection.hpp>
#include <src/Graphics/BxDFs/MicrofacetReflection.hpp>
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
namespace material {
    class ClearcoatMaterial : public IMaterial {
    public:
        bool Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const {
            if (out_bsdf) {
                out_bsdf->Add<bxdf::LambertReflection>(m_albedo, input.normal);

                out_bsdf->Add<bxdf::MicrofacetReflection>(glm::vec3(0.04f), m_roughness, input.normal);

                out_bsdf->Add<bxdf::MicrofacetReflection>(
                    glm::vec3(0.04f * m_clearcoat), m_clearcoat_roughness, input.normal); // 0.01 roughness
            }
            return true;
        }
        bool IsOpaque() const override { return true; }

        glm::vec3 m_albedo = { 1, 1, 1 };
        float m_roughness = 0.5f;
        float m_clearcoat = 1.0f;
        float m_clearcoat_roughness = 0.01f;
    };
} // namespace material
} // namespace devs_out_of_bounds