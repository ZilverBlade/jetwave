#pragma once
#define GLM_ENABLE_EXPERIMENTAL 1
//#define GLM_FORCE_AVX2 1
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES 1
//#define GLM_FORCE_INTRINSICS 1
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>

/*
 * Architecture and bitness detection
 *
 * Define one of DOOB_PLATFORM_32_BIT / DOOB_PLATFORM_64_BIT and a set of
 * architecture flags such as DOOB_ARCH_X86, DOOB_ARCH_X86_64, DOOB_ARCH_ARM,
 * DOOB_ARCH_ARM64, DOOB_ARCH_RISCV. Platform-family helpers (DOOB_PLATFORM_X86,
 * DOOB_PLATFORM_ARM) are provided for convenience.
 */

#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__) || defined(__64BIT__) ||      \
    defined(__LP64__) || defined(_LP64)
#define DOOB_PLATFORM_64_BIT 1
#else
#define DOOB_PLATFORM_32_BIT 1
#endif

/* Architecture families */
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
#define DOOB_ARCH_X86_64 1
#endif
#if defined(__i386__) || defined(_M_IX86)
#define DOOB_ARCH_X86 1
#endif
#if defined(__aarch64__) || defined(_M_ARM64)
#define DOOB_ARCH_ARM64 1
#endif
#if defined(__arm__) || defined(_M_ARM)
#define DOOB_ARCH_ARM 1
#endif
#if defined(__riscv)
#define DOOB_ARCH_RISCV 1
#endif
#if defined(__powerpc__) || defined(__ppc__) || defined(__PPC__)
#define DOOB_ARCH_PPC 1
#endif

/* Platform family aliases */
#if defined(DOOB_ARCH_X86) || defined(DOOB_ARCH_X86_64)
#define DOOB_PLATFORM_X86 1
#endif
#if defined(DOOB_ARCH_ARM) || defined(DOOB_ARCH_ARM64)
#define DOOB_PLATFORM_ARM 1
#endif

/*
 * Operating system/platform detection
 */
#if defined(__ANDROID__)
#define DOOB_PLATFORM_ANDROID 1
#define DOOB_PLATFORM_FAMILY_UNIX 1
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define DOOB_PLATFORM_WINDOWS 1
#define DOOB_PLATFORM_FAMILY_WINDOWS 1
#endif

#if defined(__linux__)
#define DOOB_PLATFORM_LINUX 1
#define DOOB_PLATFORM_FAMILY_UNIX 1
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define DOOB_PLATFORM_MACOS 1
#define DOOB_PLATFORM_FAMILY_APPLE 1
#define DOOB_PLATFORM_FAMILY_UNIX 1
#endif

#if defined(__APPLE__) && defined(__IPHONEOS__)
#define DOOB_PLATFORM_IOS 1
#define DOOB_PLATFORM_FAMILY_APPLE 1
#define DOOB_PLATFORM_FAMILY_UNIX 1
#endif

#define DOOB_NODISCARD [[nodiscard]]
#define DOOB_UNUSED [[maybe_unused]]

#if defined(_MSC_VER)
#define DOOB_FORCEINLINE __forceinline
#else
#define DOOB_FORCEINLINE inline __attribute__((always_inline))
#endif

struct NoCopy {
    NoCopy() = default;
    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;
};

struct NoMove {
    NoMove() = default;
    NoMove(NoMove&&) = delete;
    NoMove& operator=(NoMove&&) = delete;
};

struct Size {
    uint32_t width = 0;
    uint32_t height = 0;

    DOOB_NODISCARD DOOB_FORCEINLINE bool operator==(const Size&) const = default;
    DOOB_NODISCARD DOOB_FORCEINLINE bool operator!=(const Size&) const = default;
};
struct Point {
    int32_t x = 0;
    int32_t y = 0;

    DOOB_NODISCARD DOOB_FORCEINLINE bool operator==(const Point&) const = default;
    DOOB_NODISCARD DOOB_FORCEINLINE bool operator!=(const Point&) const = default;
};
struct Sizef {
    float width = 0.0f;
    float height = 0.0f;

    DOOB_NODISCARD DOOB_FORCEINLINE bool operator==(const Sizef&) const = default;
    DOOB_NODISCARD DOOB_FORCEINLINE bool operator!=(const Sizef&) const = default;
};
struct Pointf {
    float x = 0.0f;
    float y = 0.0f;

    DOOB_NODISCARD DOOB_FORCEINLINE bool operator==(const Pointf&) const = default;
    DOOB_NODISCARD DOOB_FORCEINLINE bool operator!=(const Pointf&) const = default;
};

template <typename>
struct FunctionPtr_;

// partial specialization for function types
template <typename Ret, typename... Args>
struct FunctionPtr_<Ret(Args...)> {
    using Type = Ret (*)(Args...);
};

// convenient alias
template <typename Signature>
using FunctionPtr = typename FunctionPtr_<Signature>::Type;

using Pixel = uint32_t;
#define DOOB_WRITE_PIXEL(r, g, b, a)                                                                                   \
    Pixel(((static_cast<uint32_t>(r) & 0xFFU) << 24) | ((static_cast<uint32_t>(g) & 0xFFU) << 16) |                    \
          ((static_cast<uint32_t>(b) & 0xFFU) << 8) | (static_cast<uint32_t>(a)) & 0xFFU)
#define DOOB_READ_PIXEL_R(p) static_cast<uint32_t>(((p) >> 24) & 0xFFU)
#define DOOB_READ_PIXEL_G(p) static_cast<uint32_t>(((p) >> 16) & 0xFFU)
#define DOOB_READ_PIXEL_B(p) static_cast<uint32_t>(((p) >> 8) & 0xFFU)
#define DOOB_READ_PIXEL_A(p) static_cast<uint32_t>(((p)) & 0xFFU)

#define DOOB_WRITE_PIXEL_F32(r, g, b, a) DOOB_WRITE_PIXEL((r) * 255.0f, (g) * 255.0f, (b) * 255.0f, (a) * 255.0f)


#define DOOB_MAKE_ENUM_FLAGS(Enum, Int)                                                                                \
    static Enum operator|(Enum lhs, Enum rhs) {                                                                        \
        return static_cast<Enum>(static_cast<Int>(lhs) | static_cast<Int>(rhs));                                       \
    }                                                                                                                  \
    static Enum operator&(Enum lhs, Enum rhs) {                                                                        \
        return static_cast<Enum>(static_cast<Int>(lhs) & static_cast<Int>(rhs));                                       \
    }                                                                                                                  \
    static Enum operator^(Enum lhs, Enum rhs) {                                                                        \
        return static_cast<Enum>(static_cast<Int>(lhs) ^ static_cast<Int>(rhs));                                       \
    }                                                                                                                  \
    static Enum& operator|=(Enum& lhs, Enum rhs) {                                                                     \
        lhs = lhs | rhs;                                                                                               \
        return lhs;                                                                                                    \
    }                                                                                                                  \
    static Enum& operator&=(Enum& lhs, Enum rhs) {                                                                     \
        lhs = lhs & rhs;                                                                                               \
        return lhs;                                                                                                    \
    }                                                                                                                  \
    static Enum& operator^=(Enum& lhs, Enum rhs) {                                                                     \
        lhs = lhs ^ rhs;                                                                                               \
        return lhs;                                                                                                    \
    }