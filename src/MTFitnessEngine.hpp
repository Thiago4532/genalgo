#ifndef GENALGO_MTFITNESSENGINE_HPP
#define GENALGO_MTFITNESSENGINE_HPP

#include "FitnessEngine.hpp"

#include "Vec.hpp"

GA_NAMESPACE_BEGIN

class MTFitnessEngine final : public FitnessEngine {
public:
    MTFitnessEngine();
    ~MTFitnessEngine();

    virtual const char* getEngineName() const noexcept override {
        return "MTFitnessEngine";
    }

    void evaluate_impl(std::vector<Individual>& individuals) override;
private:
    Vec3d* src;
    Vec3d* dst;
};

GA_NAMESPACE_END

#endif // GENALGO_STFITNESSENGINE_HPP
