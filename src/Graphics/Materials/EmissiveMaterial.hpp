#pragma once
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
namespace material {
    class EmissiveMaterial : public IMaterial {
    public:
        void Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const override {
            if (out_emission) {
                *out_emission = m_color * m_lumens / (4.f * glm::pi<float>());
            }
        }

        glm::vec3 m_color = { 1, 1, 1 };
        float m_lumens = 1000.f;
    };
} // namespace material
} // namespace devs_out_of_bounds