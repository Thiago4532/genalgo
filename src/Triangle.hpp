#ifndef GENALGO_TRIANGLE_HPP
#define GENALGO_TRIANGLE_HPP

#include "Color.hpp"
#include "Point.hpp"
#include "base.hpp"
#include "JSONSerializer.hpp"

GA_NAMESPACE_BEGIN

struct Triangle {
    Point<i32> a, b, c;
    Color color;

    i64 area() const;

    i64 squareDistance(Triangle const& other) const;

    bool mutateFineColor();
    bool mutateFineMoveX();
    bool mutateFineMoveY();
    bool mutateFineScale();
    bool mutateFineRotate();

    bool mutate();

    void merge(Triangle const& other);

    friend void serialize(JSONSerializerState& state, Triangle const& self);
    friend void deserialize(JSONDeserializerState& state, Triangle& self);
};

GA_NAMESPACE_END

#endif // GENALGO_TRIANGLE_HPP
