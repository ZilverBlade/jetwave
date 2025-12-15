#pragma once

#include <src/Core.hpp>
#include <src/Graphics/Fragment.hpp>
#include <src/Graphics/BSDF.hpp>

namespace devs_out_of_bounds {
struct IMaterial {
    IMaterial() = default;
    virtual ~IMaterial() = default;
    DOOB_NODISCARD virtual void Evaluate(const Fragment& input, BSDF* out_bsdf, glm::vec3* out_emission) const = 0;
};
} // namespace devs_out_of_bounds