#pragma once
#include <src/Graphics/BxDFs/OrenNayarBrdf.hpp>
#include <src/Graphics/BxDFs/GgxMicrofacetBrdf.hpp>
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
namespace material {
    class BasicOrenMaterial : public IMaterial {
    public:
        void Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const override {
            if (out_bsdf) {
                glm::vec3 nor = glm::normalize(input.normal);
                out_bsdf->Add<bxdf::OrenNayarBrdf>(m_albedo, m_diffuse_roughness_angle, nor);
                out_bsdf->Add<bxdf::GgxMicrofacetBrdf>(glm::vec3(0.04f * m_specular), m_specular_roughness, nor);
            }
        }

        glm::vec3 m_albedo = { 1, 1, 1 };
        float m_specular_roughness = 0.5f;
        float m_diffuse_roughness_angle = glm::half_pi<float>();
        float m_specular = 1.0f;
    };
} // namespace material
} // namespace devs_out_of_bounds