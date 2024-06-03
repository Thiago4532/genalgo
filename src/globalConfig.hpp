#ifndef GENALGO_GLOBAL_CONFIG_HPP
#define GENALGO_GLOBAL_CONFIG_HPP

#include "Image.hpp"
#include "base.hpp"

GA_NAMESPACE_BEGIN

// TODO: Possible encapsulation of configuration data
struct GlobalConfig {
    // Target image 
    Image targetImage;

    // Number of individuals in the population
    i32 populationSize = 100;

    // Number of triangles in each individual
    i32 numTriangles = 10;
    i32 maxTriangles = 5000; 

    // Mutation parameters
    //   * Probabilities are mutually exclusive, they must sum to <= 1
    f64 mutationChanceAdd = 0.05;
    f64 mutationChanceRemove = 0.05;
    f64 mutationChanceSwap = 0.05;
    f64 mutationChanceShape = 0.85;

    // Shape-specific mutation parameters 
    //   * Probabilities are mutually exclusive, they must sum to <= 1
    f64 mutationShapeFineColorChance = 0.6;
    f64 mutationShapeFineMoveXChance = 0.1;
    f64 mutationShapeFineMoveYChance = 0.1;
    f64 mutationShapeFineScaleChance = 0.1;

    // Renderer parameters
    i32 renderScale = 6;
    u32 renderPeriod = 50; // Number of generations between renders

    // Seed for the random number generator
    u32 seed = 0x42424242;
};

extern GlobalConfig globalCfg;

GA_NAMESPACE_END

#endif // GENALGO_GLOBAL_CONFIG_HPP
