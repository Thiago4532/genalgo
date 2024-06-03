#ifndef GENALGO_FITNESSENGINE_HPP
#define GENALGO_FITNESSENGINE_HPP

#include "base.hpp"
#include "Individual.hpp"
#include <vector>

GA_NAMESPACE_BEGIN

class FitnessEngine {
public:
    FitnessEngine() noexcept = default;
    virtual ~FitnessEngine() = default;

    virtual void evaluate(std::vector<Individual>& individuals) = 0;
};

GA_NAMESPACE_END

#endif // GENALGO_FITNESSENGINE_HPP
