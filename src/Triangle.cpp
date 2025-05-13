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

static void clamp_inplace(Point<i32>& p) {
    p.x = std::clamp(p.x, 0, globalCfg.targetImage.getWidth() - 1);
    p.y = std::clamp(p.y, 0, globalCfg.targetImage.getHeight() - 1);
}

static void clamp_inplace(Triangle& t) {
    clamp_inplace(t.a);
    clamp_inplace(t.b);
    clamp_inplace(t.c);
}


i64 Triangle::squareDistance(Triangle const& other) const {
    Point<i32> center = (a + b + c) / 3;
    Point<i32> otherCenter = (other.a + other.b + other.c) / 3;

    i64 dx = center.x - otherCenter.x;
    i64 dy = center.y - otherCenter.y;
    return dx * dx + dy * dy;
}

bool Triangle::collidesWith(Triangle const& other) const {
    // Use Separating Axis Theorem for triangle-triangle collision
    auto project = [](const Point<i32>& p, const Point<i32>& axis) -> i64 {
        return static_cast<i64>(p.x) * axis.x + static_cast<i64>(p.y) * axis.y;
    };

    auto testAxes = [&](const Point<i32> axes[3]) {
        for (int i = 0; i < 3; ++i) {
            const auto& axis = axes[i];
            // Project this triangle
            i64 aProj[3] = { project(a, axis), project(b, axis), project(c, axis) };
            i64 minA = std::min({aProj[0], aProj[1], aProj[2]});
            i64 maxA = std::max({aProj[0], aProj[1], aProj[2]});
            // Project other triangle
            i64 bProj[3] = { project(other.a, axis), project(other.b, axis), project(other.c, axis) };
            i64 minB = std::min({bProj[0], bProj[1], bProj[2]});
            i64 maxB = std::max({bProj[0], bProj[1], bProj[2]});
            // If there is a gap, no collision
            if (maxA < minB || maxB < minA)
                return false;
        }
        return true;
    };

    // Compute edge normals (axes) for both triangles
    Point<i32> axes1[3] = {
        Point<i32>{-(b.y - a.y),  b.x - a.x},
        Point<i32>{-(c.y - b.y),  c.x - b.x},
        Point<i32>{-(a.y - c.y),  a.x - c.x}
    };
    Point<i32> axes2[3] = {
        Point<i32>{-(other.b.y - other.a.y), other.b.x - other.a.x},
        Point<i32>{-(other.c.y - other.b.y), other.c.x - other.b.x},
        Point<i32>{-(other.a.y - other.c.y), other.a.x - other.c.x}
    };

    // Collide if no separating axis is found
    return testAxes(axes1) && testAxes(axes2);
}

static i64 cross(const Point<i32>& o, const Point<i32>& a, const Point<i32>& b) {
    return 1ll * (a.x - o.x) * (b.y - o.y) - 1ll * (a.y - o.y) * (b.x - o.x);
}

static i64 dot(const Point<i32>& o, const Point<i32>& a, const Point<i32>& b) {
    return 1ll * (a.x - o.x) * (b.x - o.x) + 1ll * (a.y - o.y) * (b.y - o.y);
}

static f64 getAngle(const Point<i32>& a, const Point<i32>& b, const Point<i32>& c) {
    auto dp = dot(a, b, c);
    auto cp = cross(a, b, c);
    f64 angle = std::atan2(std::abs(cp), dp);
    return angle;
}

std::pair<f64, f64> Triangle::getAnglePair() const {
    f64 angleA = getAngle(a, b, c);
    f64 angleB = getAngle(b, c, a);
    f64 angleC = getAngle(c, a, b);
    return {std::min({angleA, angleB, angleC}), std::max({angleA, angleB, angleC})};
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

bool Triangle::mutateFineColor() {
    color.r = fineAdjustColor(color.r, 25);
    color.g = fineAdjustColor(color.g, 25);
    color.b = fineAdjustColor(color.b, 25);
    color.a = fineAdjustColor(color.a, 25);
    return true;
}

bool Triangle::mutateFineMoveX() {
    i32 dx = fineAdjust(0.1 * globalCfg.targetImage.getWidth());
    a.x += dx;
    b.x += dx;
    c.x += dx;
    clamp_inplace(*this);
    return true;
}

bool Triangle::mutateFineMoveY() {
    i32 dy = fineAdjust(0.1 * globalCfg.targetImage.getHeight());
    a.y += dy;
    b.y += dy;
    c.y += dy;
    clamp_inplace(*this);
    return true;
}

bool Triangle::mutateFineScale() {
    f64 scale = randomF64(0.6, 1.8);

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

    f64 angle = randomF64(-1.1, 1.1);

    // Rotate the triangle around its center
    Point<f64> center = (a + b + c) / 3.0;
    a = center + rotate(a - center, angle);
    b = center + rotate(b - center, angle);
    c = center + rotate(c - center, angle);
    clamp_inplace(*this);
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
    f64 t = randomF64(0.2, 0.8);

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
