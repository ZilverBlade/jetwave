#pragma once
#include <src/Graphics/ITextureView.hpp>

namespace devs_out_of_bounds {
namespace texture_view {
    class Rgba8SrgbTextureView : public ITextureView {
        static constexpr int PIXEL_STRIDE = 4;

    public:
        Rgba8SrgbTextureView(const uint8_t* pixels, uint32_t width, uint32_t height, uint32_t pitch)
            : m_pixels(pixels), m_width(width), m_height(height), m_pitch(pitch) {}

        DOOB_NODISCARD glm::vec4 Read(uint32_t x, uint32_t y) const override {
            assert(x < m_width && y < m_height && "Texture sampling out of bounds!");

            const uint8_t* start = m_pixels + m_pitch * y + x * PIXEL_STRIDE;

            const float r = glm::pow(static_cast<float>(start[0]) / 255.0f, 2.2f);
            const float g = glm::pow(static_cast<float>(start[1]) / 255.0f, 2.2f);
            const float b = glm::pow(static_cast<float>(start[2]) / 255.0f, 2.2f);
            const float a = static_cast<float>(start[3]) / 255.0f;

            return { r, g, b, a };
        }
        DOOB_NODISCARD uint32_t GetWidth() const override { return m_width; }
        DOOB_NODISCARD uint32_t GetHeight() const override { return m_height; }

    private:
        uint32_t m_width, m_height;
        uint32_t m_pitch;
        const uint8_t* m_pixels;
    };
} // namespace texture_view
} // namespace devs_out_of_bounds