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

    virtual const char* getEngineName() const noexcept override {
        return "GLFitnessEngine";
    }

    void evaluate(std::vector<Individual>& individuals) override;
private:
    class Engine;
    std::unique_ptr<Engine> impl;
};

GA_NAMESPACE_END

#endif // GENALGO_GLFITNESSENGINE_HPP
