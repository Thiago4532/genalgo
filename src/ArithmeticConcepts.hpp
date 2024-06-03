#ifndef GENALGO_ARITHMETICCONCEPTS_HPP
#define GENALGO_ARITHMETICCONCEPTS_HPP

#include "base.hpp"
#include <concepts>

GA_NAMESPACE_BEGIN

template <typename T, typename U>
concept Addable = requires(T a, U b) { a + b; };
template <typename T, typename U>
concept Subtractable = requires(T a, U b) { a - b; };
template <typename T, typename U>
concept Multiplicable = requires(T a, U b) { a * b; };
template <typename T, typename U>
concept Divisible = requires(T a, U b) { a / b; };

template <typename T, typename U>
concept InlineAddable = requires(T a, U b) {
    { a += b } -> std::same_as<T&>;
};

template <typename T, typename U>
concept InlineSubtractable = requires(T a, U b) {
    { a -= b } -> std::same_as<T&>;
};

template <typename T, typename U>
concept InlineMultiplicable = requires(T a, U b) {
    { a *= b } -> std::same_as<T&>;
};

template <typename T, typename U>
concept InlineDivisible = requires(T a, U b) {
    { a /= b } -> std::same_as<T&>;
};

GA_NAMESPACE_END

#endif // GENALGO_ARITHMETICCONCEPTS_HPP

