#ifndef GENALGO_VEC_HPP
#define GENALGO_VEC_HPP

#include "base.hpp"

GA_NAMESPACE_BEGIN

namespace impl_vec {

template <i32 N, typename T>
struct Vec;

template <typename T>
struct Vec<2, T> {
    T x;
    T y;

    Vec() = default;
    Vec(T const& x, T const& y) noexcept
        : x(x), y(y) {}
};

template <typename T>
struct Vec<3, T> {
    union { T x, r; };
    union { T y, g; };
    union { T z, b; };

    Vec() = default;
    Vec(T const& x, T const& y, T const& z) noexcept
        : x(x), y(y), z(z) {}
};

template <typename T>
struct Vec<4, T> {
    union { T x, r; };
    union { T y, g; };
    union { T z, b; };
    union { T w, a; };

    Vec() = default;
    Vec(T const& x, T const& y, T const& z, T const& w) noexcept
        : x(x), y(y), z(z), w(w) {}
};

template <i32 N, typename T>
const T* as_array(const Vec<N, T>& vec) {
    static_assert(sizeof(Vec<N, T>) == N * sizeof(T)
        && alignof(Vec<N, T>) == alignof(T), "Vec must be tightly packed");
    return reinterpret_cast<const T*>(&vec);
}

template <i32 N, typename T>
T* as_array(Vec<N, T>& vec) {
    static_assert(sizeof(Vec<N, T>) == N * sizeof(T)
        && alignof(Vec<N, T>) == alignof(T), "Vec must be tightly packed");
    return reinterpret_cast<T*>(&vec);
}

template <i32 N, typename T>
Vec<N, T>& from_array(T* array) {
    static_assert(sizeof(Vec<N, T>) == N * sizeof(T)
        && alignof(Vec<N, T>) == alignof(T), "Vec must be tightly packed");
    return *reinterpret_cast<Vec<N, T>*>(array);
}

template <i32 N, typename T>
const Vec<N, T>& from_array(const T* array) {
    static_assert(sizeof(Vec<N, T>) == N * sizeof(T)
        && alignof(Vec<N, T>) == alignof(T), "Vec must be tightly packed");
    return *reinterpret_cast<const Vec<N, T>*>(array);
}

template <i32 N, typename T>
auto operator+(const Vec<N, T>& lhs, const Vec<N, T>& rhs) {
    const T* lhs_ = as_array(lhs);
    const T* rhs_ = as_array(rhs);
    T result[N];
    for (i32 i = 0; i < N; ++i) {
        result[i] = lhs_[i] + rhs_[i];
    }

    return from_array<N, T>(result);
}

template <i32 N, typename T>
auto operator-(const Vec<N, T>& lhs, const Vec<N, T>& rhs) {
    const T* lhs_ = as_array(lhs);
    const T* rhs_ = as_array(rhs);
    T result[N];
    for (i32 i = 0; i < N; ++i) {
        result[i] = lhs_[i] - rhs_[i];
    }

    return from_array<N, T>(result);
}

template <i32 N, typename T>
auto operator*(const Vec<N, T>& lhs, T const& rhs) {
    const T* lhs_ = as_array(lhs);
    T result[N];
    for (i32 i = 0; i < N; ++i) {
        result[i] = lhs_[i] * rhs;
    }

    return from_array<N, T>(result);
}

template <i32 N, typename T>
auto operator*(T const& lhs, const Vec<N, T>& rhs) {
    const T* rhs_ = as_array(rhs);
    T result[N];
    for (i32 i = 0; i < N; ++i) {
        result[i] = lhs * rhs_[i];
    }

    return from_array<N, T>(result);
}

template <i32 N, typename T>
auto operator/(const Vec<N, T>& lhs, T const& rhs) {
    const T* lhs_ = as_array(lhs);
    T result[N];
    for (i32 i = 0; i < N; ++i) {
        result[i] = lhs_[i] / rhs;
    }

    return from_array<N, T>(result);
}

template <i32 N, typename T>
Vec<N, T>& operator+=(Vec<N, T>& lhs, const Vec<N, T>& rhs) {
    T* lhs_ = as_array(lhs);
    const T* rhs_ = as_array(rhs);
    for (i32 i = 0; i < N; ++i) {
        lhs_[i] += rhs_[i];
    }

    return lhs;
}

template <i32 N, typename T>
Vec<N, T>& operator-=(Vec<N, T>& lhs, const Vec<N, T>& rhs) {
    T* lhs_ = as_array(lhs);
    const T* rhs_ = as_array(rhs);
    for (i32 i = 0; i < N; ++i) {
        lhs_[i] -= rhs_[i];
    }

    return lhs;
}

template <i32 N, typename T>
Vec<N, T>& operator*=(Vec<N, T>& lhs, T const& rhs) {
    T* lhs_ = as_array(lhs);
    for (i32 i = 0; i < N; ++i) {
        lhs_[i] *= rhs;
    }

    return lhs;
}

template <i32 N, typename T>
Vec<N, T>& operator/=(Vec<N, T>& lhs, T const& rhs) {
    T* lhs_ = as_array(lhs);
    for (i32 i = 0; i < N; ++i) {
        lhs_[i] /= rhs;
    }

    return lhs;
}

template <i32 N, typename T>
T norm(const Vec<N, T>& vec) {
    const T* vec_ = as_array(vec);
    T result {0};
    for (i32 i = 0; i < N; ++i) {
        result += vec_[i] * vec_[i];
    }

    return result;
}

} // namespace impl_vec

template <typename T>
using Vec2 = impl_vec::Vec<2, T>;
using Vec2i = Vec2<i32>;
using Vec2u = Vec2<u32>;
using Vec2f = Vec2<f32>;
using Vec2d = Vec2<f64>;

template <typename T>
using Vec3 = impl_vec::Vec<3, T>;
using Vec3i = Vec3<i32>;
using Vec3u = Vec3<u32>;
using Vec3f = Vec3<f32>;
using Vec3d = Vec3<f64>;

template <typename T>
using Vec4 = impl_vec::Vec<4, T>;
using Vec4i = Vec4<i32>;
using Vec4u = Vec4<u32>;
using Vec4f = Vec4<f32>;
using Vec4d = Vec4<f64>;

using impl_vec::norm;

GA_NAMESPACE_END

#endif // GENALGO_VEC_HPP
