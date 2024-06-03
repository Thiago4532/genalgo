#ifndef GENALGO_STFITNESSENGINE_HPP
#define GENALGO_STFITNESSENGINE_HPP

#include "FitnessEngine.hpp"

#include "Vec.hpp"

GA_NAMESPACE_BEGIN

class STFitnessEngine final : public FitnessEngine {
public:
    STFitnessEngine() = default;
    ~STFitnessEngine() override = default;

    void evaluate(std::vector<Individual>& individuals) override;
};

GA_NAMESPACE_END

#endif // GENALGO_STFITNESSENGINE_HPP
