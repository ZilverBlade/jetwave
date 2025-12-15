#pragma once
#include <src/Graphics/ISamplerState.hpp>
#include <src/Graphics/ITextureView.hpp>

namespace devs_out_of_bounds {
namespace sampler {
    class LinearWrapSampler : public ISamplerState {
    public:
        DOOB_NODISCARD glm::vec4 Sample(const ITextureView* texture, glm::vec2 tex_coord) const override {
            using namespace glm;

            tex_coord = fract(tex_coord);
            tex_coord.x += static_cast<float>(tex_coord.x < 0.0f);
            tex_coord.y += static_cast<float>(tex_coord.y < 0.0f);
            
            const vec2 res = glm::vec2(texture->GetWidth(), texture->GetHeight());

            const vec2 pos = tex_coord * res - 0.5f;
            const vec2 f = fract(pos);

            vec2 pos_top_left = floor(pos);

            vec2 tl_coord = pos_top_left + vec2(0.5f, 0.5f);
            vec2 tr_coord = pos_top_left + vec2(1.5f, 0.5f);
            vec2 bl_coord = pos_top_left + vec2(0.5f, 1.5f);
            vec2 br_coord = pos_top_left + vec2(1.5f, 1.5f);
            vec4 tl = texture->Read(static_cast<uint32_t>(tl_coord.x), static_cast<uint32_t>(tl_coord.y));
            vec4 tr = texture->Read(static_cast<uint32_t>(tr_coord.x), static_cast<uint32_t>(tr_coord.y));
            vec4 bl = texture->Read(static_cast<uint32_t>(bl_coord.x), static_cast<uint32_t>(bl_coord.y));
            vec4 br = texture->Read(static_cast<uint32_t>(br_coord.x), static_cast<uint32_t>(br_coord.y));

            vec4 ret = mix(mix(tl, tr, f.x), mix(bl, br, f.x), f.y);

            return {};
        }
    };
} // namespace sampler
} // namespace devs_out_of_bounds