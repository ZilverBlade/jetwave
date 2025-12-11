#pragma once
#include <src/Graphics/IMaterial.hpp>

namespace devs_out_of_bounds {
class TriangleDebugMaterial : public IMaterial {
public:
    TriangleDebugMaterial()
        {}

    // UV is barycentric here
    DOOB_NODISCARD MaterialOutput Evaluate(const Fragment& input) const override {
        float a, b, c;
        a = 1.0f - input.uv.x - input.uv.y;
        b = input.uv.x;
        c = input.uv.y;
        return {
            .world_normal = input.normal,
            .albedo_color = glm::vec3(a, b, c),
            .specular_color = glm::vec3(1,1,1),
            .specular_power = 64.0f,
        };
    }
};
} // namespace devs_out_of_bounds