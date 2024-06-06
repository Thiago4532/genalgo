#ifndef GENALGO_COLOR_HPP
#define GENALGO_COLOR_HPP

#include "JSONDeserializer.hpp"
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
        .add("b", (int)color.b)
        .add("a", (int)color.a);
}

inline void deserialize(JSONDeserializerState& state, Color& color) {
    auto obj = state.consume_object();
    std::string key;

    bool r = false, g = false, b = false, a = false;
    while (obj.consume_key(key)) {
        if (key == "r" && !r) {
            obj.consume_value(color.r);
            r = true;
        } else if (key == "g" && !g) {
            obj.consume_value(color.g);
            g = true;
        } else if (key == "b" && !b) {
            obj.consume_value(color.b);
            b = true;
        } else if (key == "a" && !a) {
            obj.consume_value(color.a);
            a = true;
        } else {
            obj.throw_unexpected_key(key);
        }
    }

    if (!r || !g || !b || !a)
        throw json_deserialize_exception("Color: Missing required fields");
}

#endif

GA_NAMESPACE_END

#endif // GENALGO_COLOR_HPP
