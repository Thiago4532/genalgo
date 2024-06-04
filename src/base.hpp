#ifndef GENALGO_BASE_HPP
#define GENALGO_BASE_HPP

#define GA_NAMESPACE_BEGIN namespace genalgo {
#define GA_NAMESPACE_END }

// A workaround to allow using C++17 in CUDA without having
// much trouble. Although C++20 is supported, there is a bug in clang that
// I don't wanna have the trouble to fix it.
#define GA_HAS_CPP20 (__cplusplus > 201703L)

#if defined(__CUDACC__)
#define GA_CUDA __device__ __host__
#else
#define GA_CUDA
#endif

GA_NAMESPACE_BEGIN

// FIXME: Use stdint.h instead

using i8 = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long long;

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using f32 = float;
using f64 = double;

GA_NAMESPACE_END

#endif // GENALGO_BASE_HPP
