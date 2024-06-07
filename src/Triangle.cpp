#include "Triangle.hpp"

#include "Color.hpp"
#include "GlobalConfig.hpp"
#include "globalRNG.hpp"
#include <type_traits>

GA_NAMESPACE_BEGIN

static i32 fineAdjust(i32 range) {
    return randomI32(-range, range);
}

static u8 fineAdjustColor(u8 color, u8 range) {
    i32 c = color + fineAdjust(range);
    if (c < 0)
        return 0;
    if (c > 255)
        return 255;
    return c;
}

bool Triangle::mutateFineColor() {
    color.r = fineAdjustColor(color.r, 25);
    color.g = fineAdjustColor(color.g, 25);
    color.b = fineAdjustColor(color.b, 25);
    color.a = fineAdjustColor(color.a, 25);
    return true;
}

bool Triangle::mutateFineMoveX() {
    i32 dx = fineAdjust(5);
    a.x += dx;
    b.x += dx;
    c.x += dx;
    return true;
}

bool Triangle::mutateFineMoveY() {
    i32 dy = fineAdjust(5);
    a.y += dy;
    b.y += dy;
    c.y += dy;
    return true;
}

bool Triangle::mutateFineScale() {
    f64 scale = randomF64(0.9, 1.1);

    // Scale the triangle around its center
    Point<f64> center = (a + b + c) / 3.0;
    a = center + scale * (a - center);
    b = center + scale * (b - center);
    c = center + scale * (c - center);
    return true;
}

bool Triangle::mutateFineRotate() {
    auto rotate = [](Point<f64> p, f64 angle) {
        return Point<f64>{p.x * std::cos(angle) - p.y * std::sin(angle),
                          p.x * std::sin(angle) + p.y * std::cos(angle)};
    };

    f64 angle = randomF64(-0.1, 0.1);

    // Rotate the triangle around its center
    Point<f64> center = (a + b + c) / 3.0;
    a = center + rotate(a - center, angle);
    b = center + rotate(b - center, angle);
    c = center + rotate(c - center, angle);
    return true;
}

bool Triangle::mutate() {
    f64 prob = randomF64(0, 1);
    bool mutated = false;

    if (prob < globalCfg.mutationShapeFineColorChance)
        return mutateFineColor();
    prob -= globalCfg.mutationShapeFineColorChance;

    if (prob < globalCfg.mutationShapeFineMoveXChance)
        return mutateFineMoveX();
    prob -= globalCfg.mutationShapeFineMoveXChance;

    if (prob < globalCfg.mutationShapeFineMoveYChance)
        return mutateFineMoveY();
    prob -= globalCfg.mutationShapeFineMoveYChance;

    if (prob < globalCfg.mutationShapeFineScaleChance)
        return mutateFineScale();
    prob -= globalCfg.mutationShapeFineScaleChance;

    if (prob < globalCfg.mutationShapeFineRotateChance)
        return mutateFineRotate();
    prob -= globalCfg.mutationShapeFineRotateChance;

    return mutated;
}

void serialize(JSONSerializerState& state, Triangle const& self) {
    state.return_object()
        .add("a", self.a)
        .add("b", self.b)
        .add("c", self.c)
        .add("color", self.color);
}

void deserialize(JSONDeserializerState& state, Triangle& self) {
    auto obj = state.consume_object();
    std::string key;

    bool a = false, b = false, c = false, color = false;
    while (obj.consume_key(key)) {
        if (key == "a" && !a) {
            obj.consume_value(self.a);
            a = true;
        } else if (key == "b" && !b) {
            obj.consume_value(self.b);
            b = true;
        } else if (key == "c" && !c) {
            obj.consume_value(self.c);
            c = true;
        } else if (key == "color" && !color) {
            obj.consume_value(self.color);
            color = true;
        } else {
            obj.throw_unexpected_key(key);
        }
    }

    if (!a || !b || !c || !color)
        throw json_deserialize_exception("Triangle: Missing required fields");
}

GA_NAMESPACE_END
