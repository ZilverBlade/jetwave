#pragma once
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
namespace material {
    class GridCutoutMaterial : public IMaterial {
    public:
        bool Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const {
            glm::vec3 cell = glm::floor(input.position / m_grid_size);
            if ((int(cell.x + cell.y + cell.z) % 2) == 0) {
                return false;
            }
            if (out_bsdf) {
                out_bsdf->Add<bxdf::DiffuseReflection>(m_grid_foreground, input.normal);
            }
            return true;
        }
        bool IsOpaque() const override { return false; }

        float m_grid_size = 0.5f;
        glm::vec3 m_grid_foreground = { 0.7f, 0.7f, 0.7f };
    };
} // namespace material
} // namespace devs_out_of_bounds