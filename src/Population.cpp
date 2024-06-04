#include "Population.hpp"
#include "globalConfig.hpp"
#include "globalRNG.hpp"
#include <algorithm>
#include <stdexcept>
#include "FitnessEngine.hpp"
#include "JSONSerializer/vector_serializer.hpp"

GA_NAMESPACE_BEGIN

void Population::populate(i32 size, i32 numTriangles) {
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();

    std::uniform_int_distribution<i32> xDist(0, width - 1);
    std::uniform_int_distribution<i32> yDist(0, height - 1);
    std::uniform_int_distribution<u8> colorDist;

    while(individuals.size() < size) {
        Individual& i = individuals.emplace_back();
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
    i32 size = globalCfg.targetImage.getWidth() * globalCfg.targetImage.getHeight();
    for (Individual& i : individuals) {
        i.setFitness(i.getFitness() + i.size() * (size * globalCfg.penalty));
    }
}

// TODO: Make this const again
Population Population::breed() {
    std::sort(individuals.begin(), individuals.end(), [](Individual& a, Individual& b) {
        if (a.getFitness() == b.getFitness())
            return a.size() < b.size();
        return a.getFitness() < b.getFitness();
    });

    const i32 ELITE = globalCfg.eliteSize;
    const i32 ELITE_EXTRA = globalCfg.eliteExtraSize;
    if (individuals.size() < ELITE + ELITE_EXTRA)
        throw std::runtime_error("Not enough individuals to breed");

    Population nextGen;
    nextGen.individuals.reserve(individuals.size());

    for (i32 i = 0; i < ELITE; ++i)
        nextGen.individuals.push_back(individuals[i]);

    for(i32 i = 0; i < ELITE_EXTRA; ++i) {
        Individual copy = individuals[i];
        copy.mutateRemove();
        nextGen.individuals.push_back(std::move(copy));
    }

    std::uniform_int_distribution<i32> dist(0, globalCfg.eliteBreedPoolSize - 1);

    while (nextGen.individuals.size() < individuals.size()) {
        i32 parent1 = dist(globalRNG);
        i32 parent2 = dist(globalRNG);

        Individual child = individuals[parent1].crossover(individuals[parent2]);
        nextGen.individuals.push_back(child);
    }

    return nextGen;
}

void Population::serialize(JSONSerializerState& state) const {
    state.return_value(individuals);
}

GA_NAMESPACE_END
