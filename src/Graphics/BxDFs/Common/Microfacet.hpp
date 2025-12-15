#pragma once

#include <src/Core.hpp>
#include <vector>


namespace devs_out_of_bounds {
static float D_GGX(float dotNH, float a2) {
    float f = (dotNH * a2 - dotNH) * dotNH + 1.0f;
    return a2 / glm::max(glm::pi<float>() * f * f, 1e-12f);
}

static float G1_Disney(float dotNV, float a) {
    float k = (a + 1.0f);
    k = (k * k) / 8.0f;
    return dotNV / glm::max((dotNV * (1.0f - k) + k), 1e-12f);
}

static float G1(float dotNV, float a) { 
    float k = a / 2.0f;
    return dotNV / (dotNV * (1.0f - k) + k);
}

static float G_Smith_Disney(float dotNV, float dotNL, float alpha) {
    float a2 = alpha * alpha;
    return G1(dotNV, alpha) * G1(dotNL, alpha);
}
static float G_Smith(float dotNV, float dotNL, float alpha) {
    float a2 = alpha * alpha;
    return G1(dotNV, alpha) * G1(dotNL, alpha);
}

// --- 3. Fresnel (F) : Schlick ---
static glm::vec3 F_Schlick(float dotVH, glm::vec3 F0) {
    const glm::vec3 f = (1.0f - F0) * (1.0f - dotVH);
    const glm::vec3 f2 = f * f;
    const glm::vec3 f5 = f2 * f2 * f;
    return F0 + f5;
}

} // namespace devs_out_of_bounds