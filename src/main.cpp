#include <SFML/Graphics/Image.hpp>
#include <atomic>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <source_location>
#include "FitnessEngine.hpp"
#include "GLFitnessEngine.hpp"
#include "Individual.hpp"
#include "PoorProfiler.hpp"
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
        std::cerr << "Failed to load target image\n";
        return false;
    }

    // globalCfg.seed = 651999619; // gojo satoru: triangles 10 2500 penalty 0.01
    // globalCfg.seed = 789671828; // monalisa: triangles 100 2500 penalty 0.001
    globalCfg.seed = std::random_device{}();
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
    u32 logPeriod = globalCfg.logPeriod;

    bool geneticCrash = false;
    i32 crashTimer = 100000;
    const i32 ELITE = globalCfg.eliteSize;
    const i32 ELITE_EXTRA = globalCfg.eliteExtraSize;
    const i32 ELITE_BREED_POOL = globalCfg.eliteBreedPoolSize;

    std::cout << std::fixed << std::setprecision(2);
    for (i64 nGen = 1; !shouldStop(); ++nGen) {
        sLoop.restart();
        crashTimer--;
        if (crashTimer <= 0) {
            geneticCrash ^= 1;
            if (geneticCrash) {
                crashTimer = 30000;
                globalCfg.eliteSize = 2;
                globalCfg.eliteExtraSize = 0;
                globalCfg.eliteBreedPoolSize = 2;
            } else {
                crashTimer = 100000;
                globalCfg.eliteSize = ELITE;
                globalCfg.eliteExtraSize = ELITE_EXTRA;
                globalCfg.eliteBreedPoolSize = ELITE_BREED_POOL;

                // Erase everyone after the first 5
                if (pop.getIndividuals().size() > 5)
                    pop.getIndividuals().erase(pop.getIndividuals().begin() + 5, pop.getIndividuals().end());
                pop.populate();
            }
        }

        sEvaluation.restart();
        pop.evaluate(engine);
        sEvaluation.trigger();
        for (Individual const& i : pop.getIndividuals()) {
            if (i.getFitness() < bestIndividual.getFitness()) {
                bestIndividual = i;
            } else if (i.getFitness() == bestIndividual.getFitness()) {
                if (i.size() < bestIndividual.size()) {
                    bestIndividual = i;
                }
            }
        }

        sRender.restart();
        if (nGen % renderPeriod == 0) {
            renderer.requestRender(nGen, bestIndividual, pop);
        }
        sRender.trigger();

        sBreed.restart();
        pop = pop.breed();
        sBreed.trigger();

        sLoop.trigger();
        if (nGen % logPeriod == 0) {
            std::cout << "Generation " << nGen << ' ' << (geneticCrash ? "(ON)" : "(OFF)") << '\n';
            std::cout << "Seed " << globalCfg.seed << '\n';
            
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

            auto printTime = [&](const char* name, double time, bool printPercent = true) {
                std::cout << name << ": " << 1000 * time / nGen << "ms";
                if (printPercent) {
                    std::cout << " (" << 100 * time / sLoop.elapsed() << "%)";
                }
                std::cout << '\n';
            };

            printTime("Evaluation", sEvaluation.elapsed());
            printTime(" - OpenGL", sOpenGL.elapsed());
            printTime(" - CUDA", sCuda.elapsed());
            printTime("   - Mapping", sCudaMapping.elapsed());
            printTime("   - Textures", sCudaTextures.elapsed());
            printTime("   - Copy", sCudaCopy.elapsed());
            printTime("   - Kernel", sCudaKernel.elapsed());
            printTime("   - Cleanup", sCudaCleanup.elapsed());
            printTime("Render", sRender.elapsed());
            printTime("Breed", sBreed.elapsed());
            printTime("Total", sLoop.elapsed(), false);
        }
    }

    return 0;
}

GA_NAMESPACE_END

// Entry point
int main() {
    return genalgo::main();
}
