#pragma once
#include <src/Graphics/IMaterial.hpp>
#include <src/Graphics/BxDFs/LambertBrdf.hpp>

namespace devs_out_of_bounds {
namespace material {
    class GridMaterial : public IMaterial {
    public:
        GridMaterial() {}

        bool Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const {
            if (out_bsdf) {
                glm::vec3 cell = glm::floor(input.position / m_grid_size);
                bool b_foreground = (int(cell.x + cell.y + cell.z) % 2) != 0;
                out_bsdf->Add<bxdf::LambertBrdf>(
                    b_foreground ? m_grid_foreground : m_grid_background, input.normal);
            }
            return true;
        }
        bool IsOpaque() const override { return true; }

        float m_grid_size = 0.5f;
        glm::vec3 m_grid_background = { 0.4f, 0.4f, 0.4f };
        glm::vec3 m_grid_foreground = { 0.7f, 0.7f, 0.7f };
    };
} // namespace material
} // namespace devs_out_of_bounds