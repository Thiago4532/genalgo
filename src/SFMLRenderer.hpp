#ifndef GENALGO_SFMLRENDERER_HPP
#define GENALGO_SFMLRENDERER_HPP

#include "base.hpp"
#include "Population.hpp"
#include <atomic>
#include <memory>

GA_NAMESPACE_BEGIN

class SFMLRenderer {
public:
    SFMLRenderer();
    ~SFMLRenderer(); 

    bool requestRender(i32 nGen, Individual& best, Population& pop) {
        if (renderRequested)
            return false;
        bestIndividual = best;
        population = pop;
        renderRequested = true;
        return true;
    }

    bool exited() const {
        return renderExited.load(std::memory_order_relaxed);
    }
private:
    std::atomic<bool> renderRequested;
    std::atomic<bool> renderExited;
    Individual bestIndividual;
    Population population;

    class RendererImpl;
    std::unique_ptr<RendererImpl> impl;
};

GA_NAMESPACE_END

#endif // GENALGO_SFMLRENDERER_HPP
