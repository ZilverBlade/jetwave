#pragma once

#include <src/Core.hpp>

namespace devs_out_of_bounds {
struct ITextureView;
struct ISamplerState {
    ISamplerState() = default;
    virtual ~ISamplerState() = default;
    DOOB_NODISCARD virtual glm::vec4 Sample(const ITextureView* texture, glm::vec2 tex_coord) const = 0;
    DOOB_NODISCARD glm::vec4 SampleCube(const ITextureView* texture, const glm::vec3& R) const {
        constexpr glm::vec2 invAtan = glm::vec2(0.1591f, 0.3183f);
        glm::vec2 uv = glm::vec2(glm::atan(R.z, R.x), glm::asin(R.y));
        uv *= invAtan;
        uv += 0.5;
        uv.y = 1.0f - uv.y; // top left origin
        return Sample(texture, uv);
    }
};
} // namespace devs_out_of_bounds