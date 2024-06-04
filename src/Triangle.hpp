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

    bool mutateFineColor();
    bool mutateFineMoveX();
    bool mutateFineMoveY();
    bool mutateFineScale();

    bool mutate();

    void serialize(JSONSerializerState& state) const;
};

GA_NAMESPACE_END

#endif // GENALGO_TRIANGLE_HPP
