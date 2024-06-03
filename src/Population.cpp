#include "Population.hpp"
#include "globalConfig.hpp"
#include "globalRNG.hpp"
#include <algorithm>
#include <iostream>
#include "FitnessEngine.hpp"

GA_NAMESPACE_BEGIN

void Population::populate(i32 size, i32 numTriangles) {
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();

    std::uniform_int_distribution<i32> xDist(0, width - 1);
    std::uniform_int_distribution<i32> yDist(0, height - 1);
    std::uniform_int_distribution<u8> colorDist;

    individuals.resize(size);
    for (Individual& i : individuals) {
        i.reserve(numTriangles);
        for (i32 j = 0; j < numTriangles; ++j) {
            Point p1(xDist(globalRNG), yDist(globalRNG));
            Point p2(xDist(globalRNG), yDist(globalRNG));
            Point p3(xDist(globalRNG), yDist(globalRNG));
            Color color(
                colorDist(globalRNG),
                colorDist(globalRNG),
                colorDist(globalRNG),
                colorDist(globalRNG)
            );

            i.push_back(Triangle(p1, p2, p3, color));
        }
    }
}

void Population::populate() {
    populate(globalCfg.populationSize, globalCfg.numTriangles);
}

void Population::evaluate(FitnessEngine& engine) {
    engine.evaluate(individuals);
    // for (Individual& i : individuals) {
    //     i32 penalty = std::max(0, i.size() - 10);
    //     double ratio = 1 + 0.001 * penalty;
    //     i.setFitness(i.getFitness() * ratio);
    // }
}

// TODO: Make this const again
Population Population::breed() {
    std::sort(individuals.begin(), individuals.end(), [](Individual& a, Individual& b) {
        if (a.getFitness() == b.getFitness())
            return a.size() < b.size();
        return a.getFitness() < b.getFitness();
    });

    constexpr i32 ELITE = 10;
    if (individuals.size() < ELITE)
        return *this; // Just to prevent a crash

    Population nextGen;
    nextGen.individuals.reserve(individuals.size());

    for (i32 i = 0; i < ELITE; ++i)
        nextGen.individuals.push_back(individuals[i]);

    std::uniform_int_distribution<i32> dist(0, ELITE - 1);

    for (i32 i = ELITE; i < individuals.size(); ++i) {
        i32 parent1 = randomI32(0, individuals.size() - 1);
        i32 parent2 = randomI32(0, individuals.size() - 1);

        Individual child = individuals[parent1].crossover(individuals[parent2]);
        nextGen.individuals.push_back(child);
    }

    return nextGen;
}

GA_NAMESPACE_END
