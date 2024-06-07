#include "GlobalConfig.hpp"
#include <iostream>

GA_NAMESPACE_BEGIN

GlobalConfig globalCfg;

bool GlobalConfig::setup() {
    // Target image 
    if (!targetImage.load("image.png")) {
        std::cerr << "Failed to load target image\n";
        return false;
    }

    // File to continue from
    // inputFilename = nullptr;
    inputFilename = "outputs/output.json";

    // File to output the population
    // outputFilename = nullptr;
    outputFilename = "outputs/output.json";

    // Number of individuals in the population
    populationSize = 100;

    // Number of triangles in each individual
    numTriangles = 100;
    maxTriangles = 1100; 

    // Number of elite individuals
    eliteSize = 10;
    eliteBreedPoolSize = eliteSize;

    // Mutation parameters
    //   * Probabilities are mutually exclusive, they must sum to <= 1
    mutationChanceAdd = 0.05;
    mutationChanceRemove = 0.05;
    mutationChanceReplace = 0.05;
    mutationChanceSwap = 0.05;
    mutationChanceShape = 0.8;

    // Shape-specific mutation parameters 
    //   * Probabilities are mutually exclusive, they must sum to <= 1
    mutationShapeFineColorChance = 0.20;
    mutationShapeFineMoveXChance = 0.20;
    mutationShapeFineMoveYChance = 0.20;
    mutationShapeFineScaleChance = 0.20;
    mutationShapeFineRotateChance = 0.20;

    // Penalty for each triangle in the individual
    penalty = 0.00001;

    // Renderer parameters
    renderScale = 2;
    renderPeriod = 50; // Number of generations between renders
    
    // Number of generations between logging
    logPeriod = 50;

    // Seed for the random number generator
    seed = 0x42424242;

    return true;
}

GA_NAMESPACE_END
