#include "FitnessEngine.hpp"
#include "GlobalConfig.hpp"

GA_NAMESPACE_BEGIN

void FitnessEngine::computeWeightedFitness(std::vector<Individual>& individuals, penalty_tag::none_t) noexcept {
    for (Individual& i : individuals) {
        i.setWeightedFitness(i.getFitness());
    }
}

void FitnessEngine::computeWeightedFitness(std::vector<Individual>& individuals, penalty_tag::linear_t) noexcept {
    i32 size = globalCfg.targetImage.getWidth() * globalCfg.targetImage.getHeight();
    for (Individual& i : individuals) {
        auto fitness = i.getFitness();
        fitness *= (1.0 + i.size() * globalCfg.penalty);
        i.setWeightedFitness(fitness);
    }
}

void FitnessEngine::evaluate(std::vector<Individual>& individuals) {
    evaluate_impl(individuals);
    computeWeightedFitness(individuals, penalty_tag::none);
}

GA_NAMESPACE_END
