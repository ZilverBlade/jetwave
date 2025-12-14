#pragma once

#include <src/Core.hpp>
#include <src/Graphics/Fragment.hpp>
#include <src/Graphics/BSDF.hpp>

namespace devs_out_of_bounds {
struct IMaterial {
    IMaterial() = default;
    virtual ~IMaterial() = default;
    // returns true if the material is visible
    // If the bsdf or emission is nullptr, only visibility is evaluated
    DOOB_NODISCARD virtual bool Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const = 0;
    DOOB_NODISCARD virtual bool IsOpaque() const = 0;
};
} // namespace devs_out_of_bounds