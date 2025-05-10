#ifndef GENALGO_POINT_HPP
#define GENALGO_POINT_HPP

#include "JSONDeserializer.hpp"
#include "base.hpp"
#include "ArithmeticConcepts.hpp"
#include "JSONSerializer.hpp"

GA_NAMESPACE_BEGIN

// Operators are only available in C++20
template <typename T>
struct Point {
    T x, y;

    constexpr Point() = default;
        // : x(0), y(0) {}
    constexpr Point(T const& x, T const& y) noexcept
        : x(x), y(y) {}

#if GA_HAS_CPP20
    template<typename U>
        requires (!std::same_as<T, U>
               && std::convertible_to<U, T>)
    constexpr Point(Point<U> const& other) noexcept
        : x(other.x), y(other.y) {}

    template<typename U>
        requires (!std::same_as<T, U>
               && !std::convertible_to<U, T>
               && std::constructible_from<T, U>)
    explicit constexpr Point(Point<U> const& other) noexcept
        : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {}
#endif
};

#if GA_HAS_CPP20

template <typename T, Addable<T> U>
auto operator+(const Point<T>& lhs, const Point<U>& rhs) {
    return Point(lhs.x + rhs.x, lhs.y + rhs.y);
}

template <typename T, Subtractable<T> U>
auto operator-(const Point<T>& lhs, const Point<U>& rhs) {
    return Point(lhs.x - rhs.x, lhs.y - rhs.y);
}

template <typename T, Multiplicable<T> U>
auto operator*(const Point<T>& lhs, U&& rhs) {
    return Point(lhs.x * rhs, lhs.y * rhs);
}

template <typename T, Multiplicable<T> U>
auto operator*(U&& lhs, const Point<T>& rhs) {
    return Point(lhs * rhs.x, lhs * rhs.y);
}

template <typename T, Divisible<T> U>
auto operator/(const Point<T>& lhs, U&& rhs) {
    return Point(lhs.x / rhs, lhs.y / rhs);
}

template <typename T, InlineAddable<T> U>
Point<T>& operator+=(Point<T>& lhs, const Point<U>& rhs) {
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}

template <typename T, InlineSubtractable<T> U>
Point<T>& operator-=(Point<T>& lhs, const Point<U>& rhs) {
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}

template <typename T, InlineMultiplicable<T> U>
Point<T>& operator*=(Point<T>& lhs, U&& rhs) {
    lhs.x *= rhs;
    lhs.y *= rhs;
    return lhs;
}

template <typename T, InlineDivisible<T> U>
Point<T>& operator/=(Point<T>& lhs, U&& rhs) {
    lhs.x /= rhs;
    lhs.y /= rhs;
    return lhs;
}

inline void serialize(JSONSerializerState& state, const Point<i32>& value) {
    state.return_object()
        .add("x", value.x)
        .add("y", value.y);
}

inline void deserialize(JSONDeserializerState& state, Point<i32>& value) {
    state.consume_object(
        "x", value.x,
        "y", value.y
    );
}

#endif // GA_HAS_CPP20

GA_NAMESPACE_END

#endif // GENALGO_POINT_HPP
