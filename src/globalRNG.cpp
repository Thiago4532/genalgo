#include "globalRNG.hpp"

#include <bit>

GA_NAMESPACE_BEGIN

// struct mt19937_fake {
//     alignas(std::mt19937) std::byte data[sizeof(std::mt19937)];
// };

globalRNG_t globalRNG;
static constexpr i32 MAX_BITS = globalRNG_t::word_size;

template <std::integral T>
static i32 getMSB(T value) {
    auto value_u = static_cast<std::make_unsigned_t<T>>(value);
    return 8*sizeof(T) - std::countl_zero(value_u);
}
 
u32 randomBits(u32 n) {
    static struct {
        u32 bit = 0;
        globalRNG_t::result_type value;
    } state;

    if (n <= state.bit) {
        u32 result = state.value & ((1 << n) - 1);
        state.value >>= n;
        state.bit -= n;
        return result;
    }

    n -= state.bit;
    u32 result = state.value << n;
    state.value = globalRNG();
    state.bit = MAX_BITS - n;
    return result | (state.value >> state.bit);
}

// Optimized random boolean generator
// - Uses a single random number to generate multiple random booleans.
bool randomBool() {
    return randomBits(1);
}

u8 randomU8() {
    return randomBits(8);
}

u32 randomU32() {
    return globalRNG();
}

u32 randomU32(u32 max) {
    if (max == 0)
        return 0;

    i32 msb = getMSB(max);
    for (;;) {
        u32 result = randomBits(msb);
        if (result <= max)
            return result;
    }
}

i32 randomI32(i32 min, i32 max) {
    u32 min_u = static_cast<u32>(-min);
    u32 max_u = static_cast<u32>(max);

    return static_cast<i32>(randomU32(max_u + min_u)) + min;
}

u64 randomU64() {
    u64 high = globalRNG();
    u64 low = globalRNG();
    return (high << 32) | low;
}

u64 randomU64(u64 max) {
    if (max <= std::numeric_limits<u32>::max())
        return randomU32(max);

    u64 low = randomU32();
    u64 high = randomU32(max >> 32);
    return (high << 32) | low;
}

i64 randomI64(i64 min, i64 max) {
    u64 min_u = static_cast<u64>(-min);
    u64 max_u = static_cast<u64>(max);

    return static_cast<i64>(randomU32(max_u + min_u)) + min;
}

f32 randomF32(f32 max) {
    return randomF32(0, max);
}

f32 randomF32(f32 min, f32 max) {
    return std::uniform_real_distribution<f32>(min, max)(globalRNG);
}

f64 randomF64(f64 max) {
    return randomF64(0, max);
}

f64 randomF64(f64 min, f64 max) {
    return std::uniform_real_distribution<f64>(min, max)(globalRNG);
}

GA_NAMESPACE_END
