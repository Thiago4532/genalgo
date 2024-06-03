#include <SFML/Graphics/Image.hpp>
#include <atomic>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <source_location>
#include "FitnessEngine.hpp"
#include "GLFitnessEngine.hpp"
#include "Individual.hpp"
#include "Population.hpp"
#include "SFMLRenderer.hpp"
#include "STFitnessEngine.hpp"
#include "Vec.hpp"
#include "defer.hpp"
#include "globalConfig.hpp"
#include "Image.hpp"
#include "Point.hpp"
#include "globalRNG.hpp"

GA_NAMESPACE_BEGIN

GlobalConfig globalCfg;

static bool initializeGlobalConfig() {
    // Initialization order:
    // 1. globalCfg
    // 2. globalRNG
    // These objects must be initialized before everything else

    if (!globalCfg.targetImage.load("image.png")) {
    // if (!globalCfg.targetImage.load("maomao.jpeg")) {
        std::cerr << "Failed to load target image\n";
        return false;
    }

    // globalRNG.seed(std::random_device{}());
    globalRNG.seed(globalCfg.seed);

    return true;
}


static std::atomic<bool> g_stop = false;

// Handler for SIGINT
void sigintHandler(int) {
    g_stop.store(true, std::memory_order_relaxed);
}

int main() {
    // signal(SIGINT, sigintHandler);
    if (!initializeGlobalConfig())
        return 1;

    Population pop;
    pop.populate();

    GLFitnessEngine engine;
    // STFitnessEngine engine;
    // pop.evaluate(engine);

    // std::cout << std::fixed << std::setprecision(2);
    // for (Individual const& i : pop.getIndividuals()) {
    //     Triangle const& t = i[0];
    //     std::cout << i.size() << ' ' << i.getFitness() << '\n';
    // }
    // return 0;
    
    // Renderer must always be done in other thread, since
    // the main thread may use the GPU for computation.
    SFMLRenderer renderer;

    Individual bestIndividual;
    auto shouldStop = [&]() {
        return g_stop.load(std::memory_order_relaxed)
            || renderer.exited();
    };

    u32 renderPeriod = globalCfg.renderPeriod;

    for (i64 nGen = 1; !shouldStop(); ++nGen) {
        pop.evaluate(engine);
        for (Individual const& i : pop.getIndividuals()) {
            if (i.getFitness() < bestIndividual.getFitness()) {
                bestIndividual = i;
            } else if (i.getFitness() == bestIndividual.getFitness()) {
                if (i.size() < bestIndividual.size()) {
                    bestIndividual = i;
                }
            }
        }

        if (nGen % renderPeriod == 0) {
            renderer.requestRender(nGen, bestIndividual, pop);
        }

        if (nGen % 10 == 0) {
            std::cout << "Generation " << nGen << '\n';
            
            const char* bornType;
            switch (bestIndividual.getBornType()) {
            case Individual::BornType::None:
                bornType = "Nothing";
                break;
            case Individual::BornType::Mutation:
                bornType = "Mutation";
                break;
            case Individual::BornType::Crossover:
                bornType = "Crossover";
                break;
            case Individual::BornType::CrossMutation:
                bornType = "Cross-mutation";
                break;
            }

            std::cout << "Best fitness so far: " << bestIndividual.size() << " " <<
                bestIndividual.getFitness() << ' ' << bornType << '\n';
        }

        pop = pop.breed();
    }

    return 0;
}

GA_NAMESPACE_END

// Entry point
int main() {
    return genalgo::main();
}
