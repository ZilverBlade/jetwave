#pragma once
#include <src/Graphics/BxDFs/LambertReflection.hpp>
#include <src/Graphics/BxDFs/MicrofacetReflection.hpp>
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
namespace material {
    class BasicMaterial : public IMaterial {
    public:
        bool Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const {
            if (out_bsdf) {
                out_bsdf->Add<bxdf::LambertReflection>(m_albedo, input.normal);
                out_bsdf->Add<bxdf::MicrofacetReflection>(glm::vec3(0.04f), 0.0f, input.normal);
            }
            return true;
        }
        bool IsOpaque() const override { return true; }

        glm::vec3 m_albedo = { 1, 1, 1 };
    };
} // namespace material
} // namespace devs_out_of_bounds