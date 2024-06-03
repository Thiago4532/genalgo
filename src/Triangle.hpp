#ifndef GENALGO_TRIANGLE_HPP
#define GENALGO_TRIANGLE_HPP

#include "Color.hpp"
#include "Point.hpp"
#include "base.hpp"

GA_NAMESPACE_BEGIN

struct Triangle {
    Point<i32> a, b, c;
    Color color;

    bool mutateFineColor();
    bool mutateFineMoveX();
    bool mutateFineMoveY();
    bool mutateFineScale();

    bool mutate();
};

GA_NAMESPACE_END

#endif // GENALGO_TRIANGLE_HPP
