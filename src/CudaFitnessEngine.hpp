#ifndef GENALGO_CUDAFITNESSENGINE_HPP
#define GENALGO_CUDAFITNESSENGINE_HPP

#include "FitnessEngine.hpp"
#include "Population.hpp"
#include <memory>

GA_NAMESPACE_BEGIN

class CudaFitnessEngine final : public FitnessEngine {
public:
    CudaFitnessEngine();
    ~CudaFitnessEngine();

    virtual const char* getEngineName() const noexcept override {
        return "CudaFitnessEngine";
    }

    void evaluate_impl(std::vector<Individual>& individuals) override;
private:
    class Engine;
    std::unique_ptr<Engine> impl;
};

GA_NAMESPACE_END

#endif // GENALGO_CUDAFITNESSENGINE_HPP
