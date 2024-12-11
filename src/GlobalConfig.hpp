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
    const char* inputFilename;

    // File to output the population
    const char* outputFilename;

    // File to output the best individual
    const char* outputSVG;
    i32 svgScale;

    // Number of individuals in the population
    i32 populationSize;

    // Number of triangles in each individual
    i32 numTriangles;
    i32 maxTriangles; 

    // Number of elite individuals
    i32 eliteSize;
    i32 eliteBreedPoolSize;

    // Fitness engine
    const char* fitnessEngine;

    bool breedDisabled;

    // Mutation parameters
    //   * Probabilities are mutually exclusive, they must sum to <= 1
    f64 mutationChanceAdd;
    f64 mutationChanceRemove;
    f64 mutationChanceReplace;
    f64 mutationChanceSwap;
    f64 mutationChanceShape;

    // Shape-specific mutation parameters 
    //   * Probabilities are mutually exclusive, they must sum to <= 1
    f64 mutationShapeFineColorChance;
    f64 mutationShapeFineMoveXChance;
    f64 mutationShapeFineMoveYChance;
    f64 mutationShapeFineScaleChance;
    f64 mutationShapeFineRotateChance;

    // Penalty for each triangle in the individual
    f64 penalty;

    // Renderer parameters
    bool renderDisabled;
    i32 renderScale;
    u32 renderPeriod; // Number of generations between renders
    
    // Number of generations between logging
    u32 logPeriod;

    // Seed for the random number generator
    u32 seed; 

    bool setup(int argc, char* argv[]);
    void loadConstants();
};

extern GlobalConfig globalCfg;

GA_NAMESPACE_END

#endif // GENALGO_GLOBAL_CONFIG_HPP
