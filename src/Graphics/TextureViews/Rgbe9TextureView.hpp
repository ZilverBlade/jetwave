#pragma once
#include <src/Graphics/ITextureView.hpp>
#include <glm/gtc/packing.hpp>
namespace devs_out_of_bounds {
class Rgbe9TextureView : public ITextureView {
    static constexpr int PIXEL_STRIDE = 4;

public:
    Rgbe9TextureView(const uint8_t* pixels, uint32_t width, uint32_t height, uint32_t pitch)
        : m_pixels(pixels), m_width(width), m_height(height), m_pitch(pitch) {}

    DOOB_NODISCARD glm::vec4 Read(uint32_t x, uint32_t y) const override {
        assert(x < m_width && y < m_height && "Texture sampling out of bounds!");

        const uint8_t* start = m_pixels + m_pitch * y + x * PIXEL_STRIDE;

        return { glm::unpackF3x9_E1x5(*reinterpret_cast<const uint32_t*>(start)), 1.0f };
    }
    DOOB_NODISCARD uint32_t GetWidth() const override { return m_width; }
    DOOB_NODISCARD uint32_t GetHeight() const override { return m_height; }

private:
    uint32_t m_width, m_height;
    uint32_t m_pitch;
    const uint8_t* m_pixels;
};
} // namespace devs_out_of_bounds