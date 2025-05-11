#ifndef GENALGO_POPULATION_HPP
#define GENALGO_POPULATION_HPP

#include "JSONSerializer.hpp"
#include "base.hpp"
#include "Individual.hpp"

GA_NAMESPACE_BEGIN

class FitnessEngine;

class Population {
public:
    Population() noexcept = default;

    void populate(i32 size, i32 numTriangles);
    void populate();

    void evaluate(FitnessEngine& engine);

    std::vector<Individual> const& getIndividuals() const noexcept { return individuals; }
    std::vector<Individual>& getIndividuals() noexcept { return individuals; }

    Population produce_offspring() const;
    void select_next_generation(Population&& offspring_population);

    friend void serialize(JSONSerializerState& state, const Population& population);
    friend void deserialize(JSONDeserializerState& state, Population& population);

    i32 get_mutations_per_offspring() const;
private:
    std::vector<Individual> individuals;
    i32 generation {0};
};

GA_NAMESPACE_END

#endif // GENALGO_POPULATION_HPP
