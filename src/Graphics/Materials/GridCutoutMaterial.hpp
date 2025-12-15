#pragma once
#include <src/Graphics/BxDFs/LambertBrdf.hpp>
#include <src/Graphics/BxDFs/PassthroughBtdf.hpp>
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
namespace material {
    class GridCutoutMaterial : public IMaterial {
    public:
        void Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const override {
            if (out_bsdf) {
                glm::vec3 cell = glm::floor(input.position / m_grid_size);
                if ((int(cell.x + cell.y + cell.z) % 2) == 1) {
                    out_bsdf->Add<bxdf::LambertBrdf>(m_grid_foreground, input.normal);
                } else {
                    out_bsdf->Add<bxdf::PassthroughBtdf>(glm::vec3(1, 1, 1));
                }
            }
        }

        float m_grid_size = 0.5f;
        glm::vec3 m_grid_foreground = { 0.7f, 0.7f, 0.7f };
    };
} // namespace material
} // namespace devs_out_of_bounds