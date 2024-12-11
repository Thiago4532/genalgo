#include "Population.hpp"
#include "GlobalConfig.hpp"
#include "globalRNG.hpp"
#include <algorithm>
#include <stdexcept>
#include "FitnessEngine.hpp"
#include "JSONSerializer/vector_serializer.hpp"
#include "JSONDeserializer/vector_deserializer.hpp"

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
}

Population Population::breed() const {
    std::vector<i32> idx(individuals.size());
    for (i32 i = 0; i < individuals.size(); ++i)
        idx[i] = i;

    std::sort(idx.begin(), idx.end(), [this](i32 i, i32 j) {
        Individual const& a = individuals[i];
        Individual const& b = individuals[j];

        if (a.getWeightedFitness() == b.getWeightedFitness())
            return a.size() < b.size();
        return a.getWeightedFitness() < b.getWeightedFitness();
    });

    const i32 ELITE = globalCfg.eliteSize;
    if (individuals.size() < ELITE)
        throw std::runtime_error("Not enough individuals to breed");

    Population nextGen;
    nextGen.individuals.reserve(individuals.size());

    for (i32 i = 0; i < ELITE; ++i)
        nextGen.individuals.push_back(individuals[idx[i]]);

    std::uniform_int_distribution<i32> dist(0, globalCfg.eliteBreedPoolSize - 1);

    while (nextGen.individuals.size() < individuals.size()) {
        i32 parent1 = dist(globalRNG);
        i32 parent2 = dist(globalRNG);

        Individual child = individuals[idx[parent1]].crossover(individuals[idx[parent2]]);
        nextGen.individuals.push_back(child);
    }

    return nextGen;
}

void serialize(JSONSerializerState& state, Population const& self) {
    state.return_value(self.individuals);
}

void deserialize(JSONDeserializerState& state, Population& self) {
    state.consume(self.individuals);
}

GA_NAMESPACE_END
