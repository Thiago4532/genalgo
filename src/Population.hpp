#ifndef GENALGO_POPULATION_HPP
#define GENALGO_POPULATION_HPP

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

    Population breed();
private:
    std::vector<Individual> individuals;
};

GA_NAMESPACE_END

#endif // GENALGO_POPULATION_HPP
