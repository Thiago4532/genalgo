#include "STFitnessEngine.hpp"

#include "Color.hpp"
#include "Vec.hpp"
#include "GlobalConfig.hpp"
#include "defer.hpp"
#include <algorithm>
#include <cmath>

GA_NAMESPACE_BEGIN

static bool pointInTriangle(Point<i32> const& p, Point<i32> const& a, Point<i32> const& b, Point<i32> const& c) {
    auto sign = [](Point<i32> const& p1, Point<i32> const& p2, Point<i32> const& p3) {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    };

    bool b1, b2, b3;

    b1 = sign(p, a, b) < 0;
    b2 = sign(p, b, c) < 0;
    b3 = sign(p, c, a) < 0;

    return ((b1 == b2) && (b2 == b3));
}

static Vec3d blend(Vec3d dst, Vec3d src, u8 srcAlpha) {
    // Equivalent: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    f64 alpha = srcAlpha / 255.0;
    return alpha * src + (1.0 - alpha) * dst;
}

static Vec3d fromColor(Color c) {
    return Vec3d{1.0 * c.r, 1.0 * c.g, 1.0 * c.b};
}

static void rasterize(Vec3d dst[], Triangle const& t, i32 width, i32 height) {
    Vec3d color = fromColor(t.color);
    u8 alpha = t.color.a;

    // Determine the bounding box of the triangle
    i32 minX = std::max(0, static_cast<i32>(std::min({t.a.x, t.b.x, t.c.x})));
    i32 minY = std::max(0, static_cast<i32>(std::min({t.a.y, t.b.y, t.c.y})));
    i32 maxX = std::min(width - 1, static_cast<i32>(std::max({t.a.x, t.b.x, t.c.x})));
    i32 maxY = std::min(height - 1, static_cast<i32>(std::max({t.a.y, t.b.y, t.c.y})));

    minX = std::max(0, minX);
    minY = std::max(0, minY);
    maxX = std::min(width - 1, maxX);
    maxY = std::min(height - 1, maxY);

    // Iterate over pixels in the bounding box
    for (i32 y = minY; y <= maxY; ++y) {
        for (i32 x = minX; x <= maxX; ++x) {
            // Check if the pixel is inside the triangle
            if (pointInTriangle({x, y}, t.a, t.b, t.c)) {
                i32 index = y * width + x;
                // Blend the triangle color with the destination buffer
                dst[index] = blend(dst[index], color, alpha);
            }
        }
    }
}

static void eval(Individual& individual, Vec3d dst[], Vec3d src[], i32 width, i32 height) {
    i32 size = width * height;

    for (i32 i = 0; i < size; ++i)
        dst[i] = Vec3d{0, 0, 0};

    for (const Triangle& t : individual) {
        rasterize(dst, t, width, height);
    }

    f64 fitness = 0.0;

    for (i32 i = 0; i < size; ++i) {
        Vec3d diff = dst[i] - src[i];
        Vec3d& pixel = dst[i];

        // fitness += dst[i].x + dst[i].y + dst[i].z;
        fitness += norm(diff);

        // if (pixel.x > 0 && pixel.y > 0 && pixel.z > 0) {
        //     std::printf("pixel: %f %f %f\n", pixel.x, pixel.y, pixel.z);
        // }
    }

    individual.setFitness(fitness);
}

void STFitnessEngine::evaluate(std::vector<Individual>& individuals) {
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();
    Vec3d* src = new Vec3d[width * height];
    defer { delete[] src; };
    Vec3d* dst = new Vec3d[width * height];
    defer { delete[] dst; };

    Color* target = reinterpret_cast<Color*>(globalCfg.targetImage.getData());
    for (i32 i = 0; i < width * height; ++i) {
        src[i] = blend({0, 0, 0}, fromColor(target[i]), target[i].a);
    }

    for (Individual& i : individuals) {
        eval(i, dst, src, width, height);
    }
    computeWeightedFitness(individuals, penalty_tag::linear);
}

GA_NAMESPACE_END
