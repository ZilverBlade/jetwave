#pragma once
#include <src/Graphics/ISamplerState.hpp>
#include <src/Graphics/ITextureView.hpp>

namespace devs_out_of_bounds {
namespace sampler {
    class LinearWrapSampler : public ISamplerState {
    public:
        DOOB_NODISCARD glm::vec4 Sample(const ITextureView* texture, glm::vec2 tex_coord) const override {
            using namespace glm;

            const int width = texture->GetWidth();
            const int height = texture->GetHeight();
            const vec2 res = vec2(width, height);

            const vec2 pos = tex_coord * res - 0.5f;

            const vec2 pos_top_left = floor(pos);
            const vec2 f = pos - pos_top_left; // Equivalent to fract(pos) but robust

            auto wrap = [](int x, int size) -> uint32_t { return static_cast<uint32_t>((x % size + size) % size); };

            uint32_t x0 = wrap(static_cast<int>(pos_top_left.x), width);
            uint32_t y0 = wrap(static_cast<int>(pos_top_left.y), height);

            uint32_t x1 = wrap(static_cast<int>(pos_top_left.x) + 1, width);
            uint32_t y1 = wrap(static_cast<int>(pos_top_left.y) + 1, height);

            vec4 tl = texture->Read(x0, y0); // Top-Left
            vec4 tr = texture->Read(x1, y0); // Top-Right
            vec4 bl = texture->Read(x0, y1); // Bottom-Left
            vec4 br = texture->Read(x1, y1); // Bottom-Right

            vec4 top_mix = mix(tl, tr, f.x);
            vec4 bot_mix = mix(bl, br, f.x);
            vec4 ret = mix(top_mix, bot_mix, f.y);

            return ret;
        }
    };
} // namespace sampler
} // namespace devs_out_of_bounds