#pragma once
#include <src/Graphics/ISamplerState.hpp>
#include <src/Graphics/ITextureView.hpp>

namespace devs_out_of_bounds {
class LinearWrapSampler : public ISamplerState {
public:
    DOOB_NODISCARD glm::vec4 Sample(const ITextureView* texture, glm::vec2 tex_coord) const override {
        tex_coord = glm::fract(tex_coord);
        tex_coord.x += static_cast<float>(tex_coord.x < 0.0f);
        tex_coord.y += static_cast<float>(tex_coord.y < 0.0f);

        const glm::vec2 abs_coord = tex_coord * glm::vec2(texture->GetWidth(), texture->GetHeight()) - 0.5f;
        const glm::vec2 floor_coord = glm::floor(abs_coord);
        const glm::vec2 tl = abs_coord;
        const glm::vec2 tr = abs_coord + glm::vec2(1.0f, 0.0f);
        const glm::vec2 bl = abs_coord + glm::vec2(0.0f, 1.0f);
        const glm::vec2 br = abs_coord + glm::vec2(1.0f, 1.0f);

        return {};
    }
};
} // namespace devs_out_of_bounds