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

    virtual const char* getEngineName() const noexcept = 0;

    void evaluate(std::vector<Individual>& individuals);
protected:
    struct penalty_tag {
        static constexpr struct none_t {} none {};
        static constexpr struct linear_t {} linear {};
    };

    virtual void evaluate_impl(std::vector<Individual>& individuals) = 0;

    // Since weighted fitness is basically the same for all engines, we can implement
    // some helper functions to avoid code duplication.
    static void computeWeightedFitness(std::vector<Individual>& individuals, penalty_tag::none_t) noexcept;
    static void computeWeightedFitness(std::vector<Individual>& individuals, penalty_tag::linear_t) noexcept;
};

GA_NAMESPACE_END

#endif // GENALGO_FITNESSENGINE_HPP
