#pragma once
#include <src/Core.hpp>

DOOB_FORCEINLINE static void RandomStateAdvance(uint32_t& seed) {
    ++seed;
    seed ^= seed >> 17;
    seed *= 0xed5ad4bbU;
    seed ^= seed >> 11;
    seed *= 0xac4c1b51U;
    seed ^= seed >> 15;
    seed *= 0x31848babU;
    seed ^= seed >> 14;
}
DOOB_NODISCARD DOOB_FORCEINLINE float RandomFloatAdv(uint32_t& seed) {
    float x = static_cast<float>(seed) / static_cast<float>(0xFFFFFFFF);
    assert(x == x);
    assert(!isinf(x));
    RandomStateAdvance(seed);
    return x;
}
DOOB_NODISCARD DOOB_FORCEINLINE glm::vec3 RandomHemiAdv(const glm::vec3& normal, uint32_t& seed) {
    glm::vec3 right = glm::abs(normal.z) < 0.999f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);

    glm::vec3 tangent = glm::normalize(glm::cross(right, normal));
    glm::vec3 bitangent = glm::cross(normal, tangent);

    float r1 = RandomFloatAdv(seed);
    float r2 = RandomFloatAdv(seed);

    float phi = 2.0f * glm::pi<float>() * r1;
    float cosTheta = r2; // z component
    float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);

    glm::vec3 sampleLocal(sinTheta * std::cos(phi), // x
        sinTheta * std::sin(phi),                   // y
        cosTheta                                    // z (aligned with normal)
    );

    return sampleLocal.x * tangent + sampleLocal.y * bitangent + sampleLocal.z * normal;
}

DOOB_NODISCARD DOOB_FORCEINLINE glm::vec3 RandomCone(const glm::vec3& direction, float cos_theta_max, uint32_t& seed) {
    glm::vec3 side = glm::abs(direction.z) < 0.999f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(side, direction));
    glm::vec3 up = glm::cross(direction, right);

    float r1 = RandomFloatAdv(seed);
    float r2 = RandomFloatAdv(seed);

    float z = 1.0f + r2 * (cos_theta_max - 1.0f); 
    float phi = 2.0f * glm::pi<float>() * r1;     

    float sin_theta = std::sqrt(1.0f - z * z);

    return (std::cos(phi) * sin_theta) * right + (std::sin(phi) * sin_theta) * up + z * direction;
}