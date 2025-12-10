#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>

/*
 * Architecture and bitness detection
 *
 * Define one of DOB_PLATFORM_32_BIT / DOB_PLATFORM_64_BIT and a set of
 * architecture flags such as DOB_ARCH_X86, DOB_ARCH_X86_64, DOB_ARCH_ARM,
 * DOB_ARCH_ARM64, DOB_ARCH_RISCV. Platform-family helpers (DOB_PLATFORM_X86,
 * DOB_PLATFORM_ARM) are provided for convenience.
 */

#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__) || defined(__64BIT__) ||      \
    defined(__LP64__) || defined(_LP64)
#define DOB_PLATFORM_64_BIT 1
#else
#define DOB_PLATFORM_32_BIT 1
#endif

/* Architecture families */
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
#define DOB_ARCH_X86_64 1
#endif
#if defined(__i386__) || defined(_M_IX86)
#define DOB_ARCH_X86 1
#endif
#if defined(__aarch64__) || defined(_M_ARM64)
#define DOB_ARCH_ARM64 1
#endif
#if defined(__arm__) || defined(_M_ARM)
#define DOB_ARCH_ARM 1
#endif
#if defined(__riscv)
#define DOB_ARCH_RISCV 1
#endif
#if defined(__powerpc__) || defined(__ppc__) || defined(__PPC__)
#define DOB_ARCH_PPC 1
#endif

/* Platform family aliases */
#if defined(DOB_ARCH_X86) || defined(DOB_ARCH_X86_64)
#define DOB_PLATFORM_X86 1
#endif
#if defined(DOB_ARCH_ARM) || defined(DOB_ARCH_ARM64)
#define DOB_PLATFORM_ARM 1
#endif

/*
 * Operating system/platform detection
 */
#if defined(__ANDROID__)
#define DOB_PLATFORM_ANDROID 1
#define DOB_PLATFORM_FAMILY_UNIX 1
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define DOB_PLATFORM_WINDOWS 1
#define DOB_PLATFORM_FAMILY_WINDOWS 1
#endif

#if defined(__linux__)
#define DOB_PLATFORM_LINUX 1
#define DOB_PLATFORM_FAMILY_UNIX 1
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define DOB_PLATFORM_MACOS 1
#define DOB_PLATFORM_FAMILY_APPLE 1
#define DOB_PLATFORM_FAMILY_UNIX 1
#endif

#if defined(__APPLE__) && defined(__IPHONEOS__)
#define DOB_PLATFORM_IOS 1
#define DOB_PLATFORM_FAMILY_APPLE 1
#define DOB_PLATFORM_FAMILY_UNIX 1
#endif

#define DOB_NODISCARD [[nodiscard]]
#define DOB_UNUSED [[maybe_unused]]

#if defined(_MSC_VER)
#define DOB_FORCEINLINE __forceinline
#else
#define DOB_FORCEINLINE inline __attribute__((always_inline))
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

    DOB_NODISCARD DOB_FORCEINLINE bool operator==(const Size&) const = default;
    DOB_NODISCARD DOB_FORCEINLINE bool operator!=(const Size&) const = default;
};
struct Point {
    int32_t x = 0;
    int32_t y = 0;

    DOB_NODISCARD DOB_FORCEINLINE bool operator==(const Point&) const = default;
    DOB_NODISCARD DOB_FORCEINLINE bool operator!=(const Point&) const = default;
};
struct Sizef {
    float width = 0.0f;
    float height = 0.0f;

    DOB_NODISCARD DOB_FORCEINLINE bool operator==(const Sizef&) const = default;
    DOB_NODISCARD DOB_FORCEINLINE bool operator!=(const Sizef&) const = default;
};
struct Pointf {
    float x = 0.0f;
    float y = 0.0f;

    DOB_NODISCARD DOB_FORCEINLINE bool operator==(const Pointf&) const = default;
    DOB_NODISCARD DOB_FORCEINLINE bool operator!=(const Pointf&) const = default;
};

using Pixel = uint32_t;
#define DOB_WRITE_PIXEL(r, g, b, a)                                                                                    \
    Pixel(((static_cast<uint32_t>(r) & 0xFFU) << 24) | ((static_cast<uint32_t>(g) & 0xFFU) << 16) |                    \
          ((static_cast<uint32_t>(b) & 0xFFU) << 8) | (static_cast<uint32_t>(a)) & 0xFFU)

#define DOB_WRITE_PIXEL_F32(r, g, b, a) DOB_WRITE_PIXEL(r * 255.0f, g * 255.0f, b * 255.0f, a * 255.0f)

#define DOB_MAKE_ENUM_FLAGS(Enum, Int)                                                                                 \
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