#pragma once

// platform_types.h
// Cross-platform fixed-width type definitions

#include <cstdint>
#include <cstddef>

// Check for C++23 <stdfloat> support
#if __cplusplus >= 202300L
  #if __has_include(<stdfloat>)
    #include <stdfloat>
    // Floating-point types with exact widths (C++23)
    using f16 = std::float16_t;
    using f32 = std::float32_t;
    using f64 = std::float64_t;
    using f128 = std::float128_t;
  #else
    // Fallback to standard types if <stdfloat> not available
    using f16 = float;    // no exact 16-bit fallback
    using f32 = float;
    using f64 = double;
    using f128 = long double; // often 80-bit or 128-bit
  #endif
#else
  // Pre-C++23 fallback
  using f16 = float;    // no exact 16-bit fallback
  using f32 = float;
  using f64 = double;
  using f128 = long double;
#endif

// Signed integer types
using s8   = std::int8_t;
using s16  = std::int16_t;
using s32  = std::int32_t;
using s64  = std::int64_t;

// Unsigned integer types
using u8   = std::uint8_t;
using u16  = std::uint16_t;
using u32  = std::uint32_t;
using u64  = std::uint64_t;

// Character types
using c8   = char;
using c16  = char16_t;
using c32  = char32_t;

// Size and pointer types
using usize = std::size_t;
using iptr  = std::intptr_t;
using uptr  = std::uintptr_t;

// Boolean type alias
using b8   = bool;

// Common constants
#include <limits>
static constexpr f32  F32_MAX_VAL = std::numeric_limits<f32>::max();
static constexpr f32  F32_MIN_VAL = std::numeric_limits<f32>::lowest();
static constexpr f64  F64_MAX_VAL = std::numeric_limits<f64>::max();
static constexpr f64  F64_MIN_VAL = std::numeric_limits<f64>::lowest();

// Helper macros
#define COUNT_OF(arr) (sizeof(arr) / sizeof((arr)[0]))

// Alignment macro
#if defined(_MSC_VER)
  #define ALIGNAS(x) __declspec(align(x))
#elif defined(__GNUC__) || defined(__clang__)
  #define ALIGNAS(x) __attribute__((aligned(x)))
#else
  #define ALIGNAS(x)
#endif
