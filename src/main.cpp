#include <SFML/Graphics/Image.hpp>
#include <atomic>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <source_location>
#include <stack>
#include "CudaFitnessEngine.hpp"
#include "FitnessEngine.hpp"
#include "GLFitnessEngine.hpp"
#include "Individual.hpp"
#include "JSONSerializer.hpp"
#include "PoorProfiler.hpp"
#include "Population.hpp"
#include "SFMLRenderer.hpp"
#include "STFitnessEngine.hpp"
#include "SignalHandler.hpp"
#include "Vec.hpp"
#include "defer.hpp"
#include "globalConfig.hpp"
#include "Image.hpp"
#include "Point.hpp"
#include "globalRNG.hpp"

GA_NAMESPACE_BEGIN

GlobalConfig globalCfg;

static bool setupConfiguration() {
    // Initialization order:
    // 1. globalCfg
    // 2. globalRNG
    // These objects must be initialized before everything else

    if (!globalCfg.targetImage.load("image.png")) {
        std::cerr << "Failed to load target image\n";
        return false;
    }

    globalCfg.seed = 651999619; // gojo satoru: triangles 10 2500 penalty 0.01
    // globalCfg.seed = 789671828; // monalisa: triangles 100 2500 penalty 0.001
    // globalCfg.seed = std::random_device{}();
    globalRNG.seed(globalCfg.seed);

    return true;
}

struct example {
    int x = 0;
    void serialize(JSONSerializerState& state) const {
        JSONArrayBuilder arr = state.return_array();
        arr.add(1);
        arr.add(2);
        arr.add("hello");
    }
};

int main() {
    Population pop;
    pop.populate();

    // GLFitnessEngine engine;
    CudaFitnessEngine engine;
    // STFitnessEngine engine;    
    
    // Display the name of the engine
    const char* engineName = engine.getEngineName();

    // Renderer must always be done in other thread, since
    // the main thread may use the GPU for computation.
    SFMLRenderer renderer;

    Individual bestIndividual;
    auto shouldStop = [&]() {
        return SignalHandler::interrupted()
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
    i64 nGen;
    for (nGen = 1; !shouldStop(); ++nGen) {
        profiler.start("loop");

        // Genetic crash: kill most individuals, reproduce just a few and repopulate
        // crashTimer--;
        // if (crashTimer <= 0) {
        //     geneticCrash ^= 1;
        //     if (geneticCrash) {
        //         crashTimer = 1000;
        //         globalCfg.eliteSize = 2;
        //         globalCfg.eliteExtraSize = 0;
        //         globalCfg.eliteBreedPoolSize = 2;
        //     } else {
        //         crashTimer = 100000;
        //         globalCfg.eliteSize = ELITE;
        //         globalCfg.eliteExtraSize = ELITE_EXTRA;
        //         globalCfg.eliteBreedPoolSize = ELITE_BREED_POOL;

        //         // Erase everyone after the first 5
        //         if (pop.getIndividuals().size() > 5)
        //             pop.getIndividuals().erase(pop.getIndividuals().begin() + 5, pop.getIndividuals().end());
        //         pop.populate();
        //     }
        // }

        profiler.start("evaluation", engineName);
        pop.evaluate(engine);
        profiler.stop("evaluation");

        for (Individual const& i : pop.getIndividuals()) {
            if (i.getFitness() < bestIndividual.getFitness()) {
                bestIndividual = i;
            } else if (i.getFitness() == bestIndividual.getFitness()) {
                if (i.size() < bestIndividual.size()) {
                    bestIndividual = i;
                }
            }
        }

        profiler.start("render", "Render");
        if (nGen % renderPeriod == 0) {
            renderer.requestRender(nGen, bestIndividual, pop);
        }
        profiler.stop("render");

        profiler.start("breed", "Breed");
        pop = pop.breed();
        profiler.stop("breed");

        profiler.stop("loop");
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

            ProfilerStopwatch& sLoop = profiler.getStopwatch("loop");
            auto printTime = [&](std::string_view name, i32 level, double time, bool printPercent = true) {
                if (level > 0) {
                    for (i32 i = 0; i < level; ++i)
                        std::cout << "  ";
                    std::cout << "- ";
                }

                std::cout << name << ": " << 1000 * time / nGen << "ms";
                if (printPercent) {
                    std::cout << " (" << 100 * time / sLoop.elapsed() << "%)";
                }
                std::cout << '\n';
            };

            std::stack<ProfilerStopwatch const*> stopwatchStack;
            auto& stopwatches = profiler.getStopwatches();
            for (i32 i = 1; i < stopwatches.size(); ++i) {
                ProfilerStopwatch const& sw = stopwatches[i];

                while (!stopwatchStack.empty() && !sw.parentIs(stopwatchStack.top()))
                    stopwatchStack.pop();

                printTime(sw.name(), stopwatchStack.size(), sw.elapsed());
                stopwatchStack.push(&sw);
            }
            printTime("Total", 0, sLoop.elapsed(), false);
        }
    }

    if (!globalCfg.outputFilename)
        return 0;
    std::ofstream file(globalCfg.outputFilename);
    if (!file) {
        std::cerr << "Failed to open file " << globalCfg.outputFilename << '\n';
        return 1;
    }

    json::serialize(file, [&](JSONSerializerState& state) {
        JSONObjectBuilder obj = state.return_object();
        obj.add("seed", globalCfg.seed);
        obj.add("generation", nGen);
        obj.add("population", pop);
    });
    return 0;
}

GA_NAMESPACE_END

// Entry point
int main(int argc, char* argv[]) {
    genalgo::SignalHandler::setup();
    if (!genalgo::setupConfiguration())
        return 1;

    return genalgo::main();
}
