#ifndef GENALGO_GLFITNESSENGINE_HPP
#define GENALGO_GLFITNESSENGINE_HPP

#include "FitnessEngine.hpp"
#include "Population.hpp"
#include <memory>

GA_NAMESPACE_BEGIN

class GLFitnessEngine final : public FitnessEngine {
public:
    GLFitnessEngine();
    ~GLFitnessEngine();

    void evaluate(std::vector<Individual>& individuals) override;
    int main(Population& pop);
private:
    class EngineImpl;
    std::unique_ptr<EngineImpl> impl;
};

GA_NAMESPACE_END

#endif // GENALGO_GLFITNESSENGINE_HPP
