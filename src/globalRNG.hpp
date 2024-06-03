#ifndef GENALGO_GLOBALRNG_HPP
#define GENALGO_GLOBALRNG_HPP

#include "base.hpp"
#include <random>

GA_NAMESPACE_BEGIN

using globalRNG_t = std::mt19937;
extern globalRNG_t globalRNG;

// Optimized implementation to generate N random bits
// N must be less than or equal to 32, otherwise the result is undefined.
u32 randomBits(u32 n);

// Optimized random boolean generator
bool randomBool();

// The following functions are not optimized, use std::uniform_*_distribution if you want
// to generate multiple random numbers at once.
u8 randomU8();
u32 randomU32();
u32 randomU32(u32 max);
i32 randomI32(i32 min, i32 max);
u64 randomU64();
u64 randomU64(u64 max);
i64 randomI64(i64 min, i64 max);
f32 randomF32(f32 max);
f32 randomF32(f32 min, f32 max);
f64 randomF64(f64 max);
f64 randomF64(f64 min, f64 max);

GA_NAMESPACE_END

#endif // GENALGO_GLOBALRNG_HPP
