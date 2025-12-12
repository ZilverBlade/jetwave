#pragma once
#include <src/Core.hpp>
#include <src/Graphics/Ray.hpp>

namespace devs_out_of_bounds {
struct LightSample {
    glm::vec3 L;  // Direction FROM surface TO light (normalized)
    glm::vec3 Li; // Incoming Radiance (Color * Intensity * Attenuation)
    float dist;   // Distance to the light (for shadow check)
};
struct ILight {
    ILight() = default;
    virtual ~ILight() = default;
    DOOB_NODISCARD virtual LightSample Sample(const glm::vec3& P, uint32_t& seed) const = 0;
};
} // namespace devs_out_of_bounds