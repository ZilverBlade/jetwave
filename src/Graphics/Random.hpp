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