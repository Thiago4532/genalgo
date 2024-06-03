#include "Triangle.hpp"

#include "Color.hpp"
#include "globalConfig.hpp"
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
    f64 scale = randomF64(0.95, 1.05);

    // Scale the triangle around its center
    Point<f64> center = (a + b + c) / 3.0;
    a = center + scale * (a - center);
    b = center + scale * (b - center);
    c = center + scale * (c - center);
    return true;
}

bool Triangle::mutate() {
    f64 prob = randomF64(0, 1);

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

    return false;
}

GA_NAMESPACE_END
