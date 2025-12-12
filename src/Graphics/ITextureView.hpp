#pragma once

#include <src/Core.hpp>

namespace devs_out_of_bounds {
struct ITextureView {
    ITextureView() = default;
    virtual ~ITextureView() = default;
    DOOB_NODISCARD virtual glm::vec4 Read(uint32_t x, uint32_t y) const = 0;
    DOOB_NODISCARD virtual uint32_t GetWidth() const = 0;
    DOOB_NODISCARD virtual uint32_t GetHeight() const = 0;
};
} // namespace devs_out_of_bounds