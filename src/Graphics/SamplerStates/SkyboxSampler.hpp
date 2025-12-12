#pragma once
#include <src/Graphics/ISamplerState.hpp>
#include <src/Graphics/ITextureView.hpp>

namespace devs_out_of_bounds {
class SkyboxSampler : public ISamplerState {
public:
    DOOB_NODISCARD glm::vec4 Sample(const ITextureView* texture, glm::vec2 tex_coord) const override {
        tex_coord.x = glm::fract(tex_coord.x);
        tex_coord.x += static_cast<float>(tex_coord.x < 0.0f);
        tex_coord.y = glm::clamp(tex_coord.y, 0.0f, 1.0f);

        const glm::vec2 res = glm::vec2(texture->GetWidth(), texture->GetHeight());
        const glm::uvec2 abs_coord = static_cast<glm::uvec2>(glm::floor(tex_coord * (res - 1.0f)));

        return texture->Read(abs_coord.x, abs_coord.y);
    }
};
} // namespace devs_out_of_bounds