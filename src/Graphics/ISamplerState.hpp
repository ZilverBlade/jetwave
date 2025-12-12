#pragma once

#include <src/Core.hpp>

namespace devs_out_of_bounds {
struct ITextureView;
struct ISamplerState {
    ISamplerState() = default;
    virtual ~ISamplerState() = default;
    DOOB_NODISCARD virtual glm::vec4 Sample(const ITextureView* texture, glm::vec2 tex_coord) const = 0;
};
} // namespace devs_out_of_bounds