#pragma once

#include <src/Core.hpp>
#include <src/Graphics/Random.hpp>
#include <type_traits>

namespace devs_out_of_bounds {
enum class BxDFType : int {
    DIFFUSE = 1 << 0,
    SPECULAR = 1 << 1,
    TRANSMISSION = 1 << 2,
};
DOOB_MAKE_ENUM_FLAGS(BxDFType, int)

struct IBxDF {
    IBxDF() = default;
    virtual ~IBxDF() = default;

    // How much light reflects from wi to wo?
    // NOTE: Returns BRDF * NdotL
    DOOB_NODISCARD virtual glm::vec3 EvaluateCos(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const = 0;

    //  What was the probability of generating wi?
    DOOB_NODISCARD virtual float Pdf(const glm::vec3& wo, const glm::vec3& wm, const glm::vec3& wi) const = 0;

    // Generate a new direction wi
    DOOB_NODISCARD virtual glm::vec3 NextSample(const glm::vec3& wo, uint32_t& seed) const = 0;

    DOOB_NODISCARD virtual BxDFType Type() const = 0;
};

template <typename T>
concept BxDFConcept = std::derived_from<T, IBxDF>;

} // namespace devs_out_of_bounds