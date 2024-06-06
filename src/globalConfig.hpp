#ifndef GENALGO_GLOBAL_CONFIG_HPP
#define GENALGO_GLOBAL_CONFIG_HPP

#include "Image.hpp"
#include "base.hpp"

GA_NAMESPACE_BEGIN

// TODO: Possible encapsulation of configuration data
struct GlobalConfig {
    // Target image 
    Image targetImage;

    // File to continue from
    const char* inputFilename = nullptr;
    // const char* inputFilename = "outputs/lisa.json";

    // File to output the population
    // const char* outputFilename = nullptr;
    const char* outputFilename = "outputs/output.json";

    // Number of individuals in the population
    i32 populationSize = 100;

    // Number of triangles in each individual
    i32 numTriangles = 100;
    i32 maxTriangles = 1100; 

    // Number of elite individuals
    i32 eliteSize = 10;
    i32 eliteExtraSize = 0; // Copy individual and remove 1 triangle
    i32 eliteBreedPoolSize = eliteSize;

    // Mutation parameters
    //   * Probabilities are mutually exclusive, they must sum to <= 1
    f64 mutationChanceAdd = 0.05;
    f64 mutationChanceRemove = 0.05;
    f64 mutationChanceReplace = 0.05;
    f64 mutationChanceSwap = 0.05;
    f64 mutationChanceShape = 0.8;

    // Shape-specific mutation parameters 
    //   * Probabilities are mutually exclusive, they must sum to <= 1
    f64 mutationShapeFineColorChance = 0.40;
    f64 mutationShapeFineMoveXChance = 0.15;
    f64 mutationShapeFineMoveYChance = 0.15;
    f64 mutationShapeFineScaleChance = 0.15;
    f64 mutationShapeFineRotateChance = 0.15;

    // Penalty for each triangle in the individual
    f64 penalty = 0.00001;

    // Renderer parameters
    i32 renderScale = 2;
    u32 renderPeriod = 50; // Number of generations between renders
    
    // Number of generations between logging
    u32 logPeriod = 50;

    // Seed for the random number generator
    u32 seed = 0x42424242;
};

extern GlobalConfig globalCfg;

GA_NAMESPACE_END

#endif // GENALGO_GLOBAL_CONFIG_HPP
