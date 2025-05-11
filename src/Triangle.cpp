#include "Triangle.hpp"

#include "Color.hpp"
#include "GlobalConfig.hpp"
#include "globalRNG.hpp"
#include <algorithm>
#include <type_traits>

GA_NAMESPACE_BEGIN

i64 Triangle::area() const {
    return std::abs((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
}

i64 Triangle::squareDistance(Triangle const& other) const {
    Point<i32> center = (a + b + c) / 3;
    Point<i32> otherCenter = (other.a + other.b + other.c) / 3;

    i64 dx = center.x - otherCenter.x;
    i64 dy = center.y - otherCenter.y;
    return dx * dx + dy * dy;
}

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

static void clamp_inplace(Point<i32>& p) {
    p.x = std::clamp(p.x, 0, globalCfg.targetImage.getWidth() - 1);
    p.y = std::clamp(p.y, 0, globalCfg.targetImage.getHeight() - 1);
}

static void clamp_inplace(Triangle& t) {
    clamp_inplace(t.a);
    clamp_inplace(t.b);
    clamp_inplace(t.c);
}

bool Triangle::mutateVertex() {
    // Select one vertex randomly
    Point<i32>* vertices[] = {&a, &b, &c};
    Point<i32>* vertex_to_mutate = vertices[randomI32(0, 2)];

    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();

    vertex_to_mutate->x = std::clamp(vertex_to_mutate->x + fineAdjust(globalCfg.vertexMutationRange), 0, width - 1);
    vertex_to_mutate->y = std::clamp(vertex_to_mutate->y + fineAdjust(globalCfg.vertexMutationRange), 0, height - 1);
    return true;
}


bool Triangle::mutateFineColor() {
    color.r = fineAdjustColor(color.r, globalCfg.fineColorAdjustRange);
    color.g = fineAdjustColor(color.g, globalCfg.fineColorAdjustRange);
    color.b = fineAdjustColor(color.b, globalCfg.fineColorAdjustRange);
    color.a = fineAdjustColor(color.a, globalCfg.fineColorAdjustRange);
    return true;
}

bool Triangle::mutateFineMoveX() {
    i32 width = globalCfg.targetImage.getWidth();
    i32 dx = fineAdjust(globalCfg.fineMoveAdjustRange * width);
    a.x = std::clamp(a.x + dx, 0, width - 1);
    b.x = std::clamp(b.x + dx, 0, width - 1);
    c.x = std::clamp(c.x + dx, 0, width - 1);
    return true;
}

bool Triangle::mutateFineMoveY() {
    i32 height = globalCfg.targetImage.getHeight();
    i32 dy = fineAdjust(globalCfg.fineMoveAdjustRange * height);
    a.y = std::clamp(a.y + dy, 0, height - 1);
    b.y = std::clamp(b.y + dy, 0, height - 1);
    c.y = std::clamp(c.y + dy, 0, height - 1);
    return true;
}

bool Triangle::mutateFineScale() {
    // TODO: Range should also be configurable
    f64 scale = randomF64(0.6, 1.4);

    // Scale the triangle around its center
    Point<f64> center = (a + b + c) / 3.0;
    a = center + scale * (a - center);
    b = center + scale * (b - center);
    c = center + scale * (c - center);

    clamp_inplace(*this);
    return true;
}

bool Triangle::mutateFineRotate() {
    auto rotate = [](Point<f64> p, f64 angle) {
        return Point<f64>{p.x * std::cos(angle) - p.y * std::sin(angle),
                          p.x * std::sin(angle) + p.y * std::cos(angle)};
    };

    f64 angle = randomF64(-0.15, 0.15);

    // Rotate the triangle around its center
    Point<f64> center = (a + b + c) / 3.0;
    a = center + rotate(a - center, angle);
    b = center + rotate(b - center, angle);
    c = center + rotate(c - center, angle);

    clamp_inplace(*this);
    return true;
}

bool Triangle::mutate() {
    bool mutated = false;

    if (randomF64(0, 1) < globalCfg.mutationShapeVertexChance)
        mutated |= mutateVertex();
    if (randomF64(0, 1) < globalCfg.mutationShapeFineColorChance)
        mutated |= mutateFineColor();
    if (randomF64(0, 1) < globalCfg.mutationShapeFineMoveXChance)
        mutated |= mutateFineMoveX();
    if (randomF64(0, 1) < globalCfg.mutationShapeFineMoveYChance)
        mutated |= mutateFineMoveY();
    if (randomF64(0, 1) < globalCfg.mutationShapeFineScaleChance)
        mutated |= mutateFineScale();
    if (randomF64(0, 1) < globalCfg.mutationShapeFineRotateChance)
        mutated |= mutateFineRotate();

    return mutated;
}

void Triangle::merge(Triangle const& other) {
    a = (a + other.a) / 2;
    b = (b + other.b) / 2;
    c = (c + other.c) / 2;

    color.r = (color.r + other.color.r) / 2;
    color.g = (color.g + other.color.g) / 2;
    color.b = (color.b + other.color.b) / 2;
    color.a = (color.a + other.color.a) / 2;
}

std::pair<Triangle, Triangle> Triangle::split() const {
    Point<i32> v0, v1, v2; // Vertices: v0 is common, v1-v2 is the side to split.

    int common_vertex_idx = randomI32(0, 2); // Choose which vertex is v0

    if (common_vertex_idx == 0) {
        v0 = a; v1 = b; v2 = c;
    } else if (common_vertex_idx == 1) {
        v0 = b; v1 = c; v2 = a; // Maintain cyclic order for v1, v2 relative to v0
    } else { // common_vertex_idx == 2
        v0 = c; v1 = a; v2 = b;
    }

    // Convert vertices of the side to be split to Point<f64> for precise interpolation.
    // Assuming Point<T> has a constructor Point<T>(const Point<U>&) or Point<T>(U x, U y)
    // and necessary arithmetic operators.
    Point<f64> v1_f64(static_cast<f64>(v1.x), static_cast<f64>(v1.y));
    Point<f64> v2_f64(static_cast<f64>(v2.x), static_cast<f64>(v2.y));

    // 't' is a random factor to pick a point on the segment v1v2.
    // A range like [0.4, 0.6] splits "around the middle".
    f64 t = randomF64(0.4, 0.6);

    // Calculate the split point s_pt_f64 = v1_f64 + t * (v2_f64 - v1_f64)
    Point<f64> s_pt_f64 = v1_f64 + (v2_f64 - v1_f64) * t;

    // Convert the split point back to Point<i32>.
    // This relies on Point<i32>'s ability to be assigned from Point<f64>
    // (e.g., via an assignment operator or constructor that handles conversion).
    Point<i32> s_pt = s_pt_f64; // Implicit conversion from Point<f64> to Point<i32>

    // Clamp the new split point to be within image boundaries.
    clamp_inplace(s_pt);

    // Create the two new triangles.
    Triangle triangle1, triangle2;

    // Triangle 1: (common vertex, first vertex of split side, split point)
    triangle1.a = v0;
    triangle1.b = v1;
    triangle1.c = s_pt;
    triangle1.color = this->color; // Inherit color

    // Triangle 2: (common vertex, split point, second vertex of split side)
    triangle2.a = v0;
    triangle2.b = s_pt;
    triangle2.c = v2;
    triangle2.color = this->color; // Inherit color
    
    // The original vertices (v0, v1, v2) are assumed to be valid.
    // s_pt is clamped. So, individual vertices of triangle1 and triangle2 should be valid.
    // If further clamping of the entire new triangles is desired, one could call:
    // clamp_inplace(triangle1);
    // clamp_inplace(triangle2);
    // However, this might be redundant if s_pt is the only new coordinate.

    return {triangle1, triangle2};
}

void serialize(JSONSerializerState& state, Triangle const& self) {
    state.serialize_object()
        .add("a", self.a)
        .add("b", self.b)
        .add("c", self.c)
        .add("color", self.color);
}

void deserialize(JSONDeserializerState& state, Triangle& self) {
    state.consume_object(
        "a", self.a,
        "b", self.b,
        "c", self.c,
        "color", self.color
    );
}

GA_NAMESPACE_END
