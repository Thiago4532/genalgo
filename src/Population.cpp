#include "Population.hpp"
#include "GlobalConfig.hpp"
#include "globalRNG.hpp"
#include <algorithm>
#include <iostream>
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
    std::uniform_int_distribution<u8> colorDist(0, 255);
    std::uniform_int_distribution<u8> alphaDist(50, 255);

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
                alphaDist(globalRNG)
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

Population Population::produce_offspring() const {
    Population offspring_population;
    offspring_population.individuals.reserve(this->individuals.size());
    offspring_population.generation = this->generation + 1;

    i32 mutationsPerOffspring = get_mutations_per_offspring();

    for (const Individual& parent_individual : this->individuals) {
        Individual offspring = parent_individual;

        for (int m = 0; m < mutationsPerOffspring; ++m) {
            
            int tries = 0;
            while(!offspring.mutate())
                tries++;

            if (tries > 10)
                std::cout << "Number of attempts until mutation: " << tries << std::endl;
        }

        offspring.setGeneration(offspring_population.generation);
        offspring_population.individuals.push_back(offspring);
    }

    return offspring_population;
}

i32 Population::get_mutations_per_offspring() const {
    i32 mutationsPerOffspring = 1;
    return std::min(mutationsPerOffspring, 20);
}

void Population::select_next_generation(Population&& offspring_population) {
    this->individuals.reserve(this->individuals.size() + offspring_population.individuals.size());

    for (Individual& offspring_ind : offspring_population.individuals) {
        this->individuals.push_back(std::move(offspring_ind));
    }

    std::sort(this->individuals.begin(), this->individuals.end(),
        [](const Individual& a, const Individual& b) {
            if (a.getWeightedFitness() == b.getWeightedFitness()) {
                return a.size() < b.size();
            }
            return a.getWeightedFitness() < b.getWeightedFitness();
        }
    );

    if (this->individuals.size() > globalCfg.populationSize) {
        this->individuals.resize(globalCfg.populationSize);
    }
    this->generation = std::max(this->generation, offspring_population.generation);
}

void serialize(JSONSerializerState& state, Population const& self) {
    state.serialize_object()
        .add("generation", self.generation)
        .add("individuals", self.individuals);
}

void deserialize(JSONDeserializerState& state, Population& self) {
    state.consume_object(
        "generation", self.generation,
        "individuals", self.individuals
    );
}

GA_NAMESPACE_END
