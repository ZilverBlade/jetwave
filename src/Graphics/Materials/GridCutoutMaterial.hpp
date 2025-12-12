#pragma once
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
class GridCutoutMaterial : public IMaterial {
public:
    GridCutoutMaterial() {}

    DOOB_NODISCARD MaterialOutput Evaluate(const Fragment& input) const override {
        return {
            .world_normal = input.normal,
            .albedo_color = m_grid_foreground,
            .specular_color = { .1f, .1f, .1f },
            .specular_power = 16.0f,
        };
    }
    DOOB_NODISCARD bool EvaluateDiscard(const Fragment& input) const override {
        glm::vec3 cell = glm::floor(input.position / m_grid_size);
        return (int(cell.x + cell.y + cell.z) % 2) == 0;
    }

    float m_grid_size = 0.5f;
    glm::vec3 m_grid_foreground = { 0.7f, 0.7f, 0.7f };
};
} // namespace devs_out_of_bounds