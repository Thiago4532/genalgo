#include "MTFitnessEngine.hpp"

#include "Color.hpp"
#include "Vec.hpp"
#include "GlobalConfig.hpp"
#include "defer.hpp"
#include <algorithm>
#include <cmath>

#include <omp.h>

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
    i32 minX = std::max(0, std::min({t.a.x, t.b.x, t.c.x}));
    i32 minY = std::max(0, std::min({t.a.y, t.b.y, t.c.y}));
    i32 maxX = std::min(width - 1, std::max({t.a.x, t.b.x, t.c.x}));
    i32 maxY = std::min(height - 1, std::max({t.a.y, t.b.y, t.c.y}));

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

        fitness += norm(diff);
    }

    individual.setFitness(fitness);
}

void MTFitnessEngine::evaluate(std::vector<Individual>& individuals){
    if (individuals.size() != globalCfg.populationSize) {
        std::fprintf(stderr, "MTFitnessEngine::evaluate: individuals.size() != populationSize\n");
        std::abort();
    }
    
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();
    
 
    constexpr i32 NUM_THREADS = 16;
    #pragma omp parallel for num_threads(NUM_THREADS)
    for (i32 i = 0; i < individuals.size(); i++) {
        eval(individuals[i], dst+(i*width*height), src, width, height);
    }

    computeWeightedFitness(individuals, penalty_tag::linear);
}

MTFitnessEngine::MTFitnessEngine(){
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();

    src = new Vec3d[width * height];  
    dst = new Vec3d[width * height * globalCfg.populationSize];

    Color* target = reinterpret_cast<Color*>(globalCfg.targetImage.getData());

    for (i32 i = 0; i < width * height; ++i) {
        src[i] = blend({0, 0, 0}, fromColor(target[i]), target[i].a);
    }
}

MTFitnessEngine::~MTFitnessEngine(){
    delete[] src;
    delete[] dst;
}

GA_NAMESPACE_END