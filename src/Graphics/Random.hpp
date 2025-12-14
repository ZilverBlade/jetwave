#pragma once
#include <src/Core.hpp>

struct UniformDistribution {
    DOOB_NODISCARD DOOB_FORCEINLINE static uint32_t RandomStateAdvance(uint32_t& seed) {
        uint32_t state = seed;
        seed = seed * 747796405u + 2891336453u;
        uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
        return (word >> 22u) ^ word;
    }
};
template <typename TRNG>
DOOB_NODISCARD DOOB_FORCEINLINE float RandomFloatAdv(uint32_t& seed) {
    uint32_t x = TRNG::RandomStateAdvance(seed);
    static const uint32_t bits = 0x2f800004u;
    const float result = float(x) * reinterpret_cast<const float&>(bits);
    assert(result == result);
    assert(!isinf(result));
    return result;
}
template <typename TRNG>
DOOB_NODISCARD DOOB_FORCEINLINE glm::vec3 RandomHemiAdv(const glm::vec3& normal, uint32_t& seed) {
    glm::vec3 right = glm::abs(normal.z) < 0.999f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);

    glm::vec3 tangent = glm::normalize(glm::cross(right, normal));
    glm::vec3 bitangent = glm::cross(normal, tangent);

    float r1 = RandomFloatAdv<TRNG>(seed);
    float r2 = RandomFloatAdv<TRNG>(seed);

    float phi = 2.0f * glm::pi<float>() * r1;
    float cosTheta = r2; // z component
    float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);

    glm::vec3 sampleLocal(sinTheta * std::cos(phi), // x
        sinTheta * std::sin(phi),                   // y
        cosTheta                                    // z (aligned with normal)
    );

    return sampleLocal.x * tangent + sampleLocal.y * bitangent + sampleLocal.z * normal;
}
template <typename TRNG>
DOOB_NODISCARD DOOB_FORCEINLINE glm::vec3 RandomCosWeightedHemiAdv(const glm::vec3& normal, uint32_t& seed) {
    glm::vec3 right = glm::abs(normal.z) < 0.999f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
    glm::vec3 tangent = glm::normalize(glm::cross(right, normal));
    glm::vec3 bitangent = glm::cross(normal, tangent);

    float r1 = RandomFloatAdv<TRNG>(seed);
    float r2 = RandomFloatAdv<TRNG>(seed);

    float phi = 2.0f * glm::pi<float>() * r1;

    float cosTheta = std::sqrt(r2);

    float sinTheta = std::sqrt(1.0f - r2);

    glm::vec3 sampleLocal(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);

    return sampleLocal.x * tangent + sampleLocal.y * bitangent + sampleLocal.z * normal;
}
template <typename TRNG>
DOOB_NODISCARD DOOB_FORCEINLINE glm::vec3 RandomConeAdv(
    const glm::vec3& direction, float cos_theta_max, uint32_t& seed) {
    glm::vec3 side = glm::abs(direction.z) < 0.999f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(side, direction));
    glm::vec3 up = glm::cross(direction, right);

    float r1 = RandomFloatAdv<TRNG>(seed);
    float r2 = RandomFloatAdv<TRNG>(seed);

    float z = 1.0f + r2 * (cos_theta_max - 1.0f);
    float phi = 2.0f * glm::pi<float>() * r1;

    float sin_theta = std::sqrt(1.0f - z * z);

    return (std::cos(phi) * sin_theta) * right + (std::sin(phi) * sin_theta) * up + z * direction;
}