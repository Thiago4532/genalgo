#ifndef GENALGO_COLOR_HPP
#define GENALGO_COLOR_HPP

#include "base.hpp"
#include "JSONSerializer.hpp"

GA_NAMESPACE_BEGIN

struct Color {
    u8 r, g, b, a;

    u32 toRGBA() const {
        return (r << 24) | (g << 16) | (b << 8) | a;
    }
};

// Ensure that Color is castable from and to u8[4]
static_assert(sizeof(Color) == 4 && alignof(Color) == 1, "Color must be 4 bytes and 1 byte aligned");

#if GA_HAS_CPP20

inline void serialize(JSONSerializerState& state, const Color& color) {
    state.return_object()
        .add("r", (int)color.r)
        .add("g", (int)color.g)
        .add("b", (int)color.b);
}

#endif

GA_NAMESPACE_END

#endif // GENALGO_COLOR_HPP
