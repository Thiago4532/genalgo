#include <SFML/Graphics/Image.hpp>
#include <atomic>
#include <csignal>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <stack>
#include "AppState.hpp"
#include "CudaFitnessEngine.hpp"
#include "FitnessEngine.hpp"
#include "Individual.hpp"
#include "JSONDeserializer.hpp"
#include "JSONSerializer.hpp"
#include "MTFitnessEngine.hpp"
#include "PoorProfiler.hpp"
#include "Population.hpp"
#include "SFMLRenderer.hpp"
#include "STFitnessEngine.hpp"
#include "SignalHandler.hpp"
#include "Vec.hpp"
#include "defer.hpp"
#include "GlobalConfig.hpp"
#include "Image.hpp"
#include "Point.hpp"
#include "globalRNG.hpp"

GA_NAMESPACE_BEGIN

static bool setupConfiguration(int argc, char* argv[]) {
    // Initialization order:
    // 1. globalCfg
    // 2. globalRNG
    // These objects must be initialized before everything else
    if (!globalCfg.setup(argc, argv))
        return false;

    globalRNG.seed(globalCfg.seed);
    return true;
}

static int readState(Population& pop, i64& nGen) {
    if (!globalCfg.inputFilename)
        return 0;

    std::ifstream input(globalCfg.inputFilename);
    if (!input) {
        if (globalCfg.outputFilename && std::strcmp(globalCfg.inputFilename, globalCfg.outputFilename) == 0) {
            std::cerr << "genalgo: Warning: Input file does not exist, but continuing anyway..." << std::endl;
            return 0;
        }
        std::cerr << "genalgo: Failed to open file: " << globalCfg.inputFilename << std::endl;
        return -1;
    }

    AppState state {
        .population = pop,
        .generation = nGen,
        .seed = globalCfg.seed
    };

    json::deserialize(input, state);
    if (state.size.x != globalCfg.targetImage.getWidth() || state.size.y != globalCfg.targetImage.getHeight()) {
        std::cerr << "genalgo: Error while loading state: Target image size mismatch" << std::endl;
        std::cerr << "         You must use the same image used in the gen-input file" << std::endl;
        return -1;
    }

    return 1;
}

int main() {
    Population pop;
    i64 nGen;

    switch (readState(pop, nGen)) {
    case 0:
        nGen = 1;
        pop.populate();
        break;
    case 1:
        break; // State loaded successfully
    case -1:
        return 1;
    }

    auto engine = []() -> std::unique_ptr<FitnessEngine> {
        std::string fitnessEngine = globalCfg.fitnessEngine;
        for (char& c : fitnessEngine)
            c = std::toupper(c);

        if (fitnessEngine == "CUDA") {
            return std::make_unique<CudaFitnessEngine>();
        } else if (fitnessEngine == "MT") {
            return std::make_unique<MTFitnessEngine>();
        } else if (fitnessEngine == "ST") {
            return std::make_unique<STFitnessEngine>();
        } else {
            std::cerr << "genalgo: Unknown fitness engine: " << globalCfg.fitnessEngine << std::endl;
            return nullptr;
        }
    }();
    if (engine == nullptr)
        return 1;
    
    // Display the name of the engine
    const char* engineName = engine->getEngineName();

    // Renderer must always be done in other thread, since
    // the main thread may use the GPU for computation.
    SFMLRenderer* renderer = nullptr;
    if (!globalCfg.renderDisabled)
        renderer = new SFMLRenderer();

    Individual bestIndividual;
    f64 oldBestFitness = std::numeric_limits<f64>::max();
    auto shouldStop = [&]() {
        return SignalHandler::interrupted()
            || (renderer ? renderer->exited() : false);
    };

    u32 renderPeriod = globalCfg.renderPeriod;
    u32 logPeriod = globalCfg.logPeriod;

    std::cout << std::fixed << std::setprecision(2);
    for (i64 cGen = 1; !shouldStop(); ++cGen, ++nGen) {
        profiler.start("loop");

        profiler.start("evaluation", engineName);
        pop.evaluate(*engine);
        profiler.stop("evaluation");

        for (Individual const& i : pop.getIndividuals()) {
            if (i.getWeightedFitness() < bestIndividual.getWeightedFitness()) {
                bestIndividual = i;
            } else if (i.getWeightedFitness() == bestIndividual.getWeightedFitness()) {
                if (i.size() < bestIndividual.size()) {
                    bestIndividual = i;
                }
            }
        }

        profiler.start("render", "Render");
        if (renderPeriod && cGen % renderPeriod == 0) {
            if (renderer)
                renderer->requestRender(nGen, bestIndividual, pop);
        }
        profiler.stop("render");

        if (!globalCfg.breedDisabled) {
            profiler.start("breed", "Breed");
            pop = pop.breed();
            profiler.stop("breed");
        }

        profiler.stop("loop");
        if (logPeriod && cGen % logPeriod == 0) {
            std::cout << "Generation " << nGen << '\n';
            std::cout << "Seed " << globalCfg.seed << '\n';
            
            f64 decrease = (oldBestFitness - bestIndividual.getFitness()) / oldBestFitness;
            oldBestFitness = bestIndividual.getFitness();

            // Print normal fitness instead of weighted fitness
            // Doing this helps to compare individuals with different weights.
            std::cout << "Best individual: " << bestIndividual.size() << " " <<
                bestIndividual.getFitness() << " (improvement = " << 100.0 * decrease << "%)\n";

            ProfilerStopwatch& sLoop = profiler.getStopwatch("loop");
            auto printTime = [&](std::string_view name, i32 level, double time, bool printPercent = true) {
                if (level > 0) {
                    for (i32 i = 0; i < level; ++i)
                        std::cout << "  ";
                    std::cout << "- ";
                }

                std::cout << name << ": " << 1000 * time / globalCfg.logPeriod << "ms";
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

            for (ProfilerStopwatch& sw : stopwatches) {
                sw.reset();
            }

            std::cout.flush();
        }
    }

    if (globalCfg.outputFilename) {
        std::ofstream output(globalCfg.outputFilename);
        if (!output) {
            std::cerr << "genalgo: Failed to open file " << globalCfg.outputFilename << std::endl;
            std::cerr << "genalgo: Unable to save state!" << std::endl;
        } else {
            json::serialize(output, AppState {
                    .population = pop,
                    .generation = nGen,
                    .seed = globalCfg.seed,
                    .size = {globalCfg.targetImage.getWidth(), globalCfg.targetImage.getHeight()}
                    });
        }
    }

    if (globalCfg.outputSVG) {
        std::ofstream stream(globalCfg.outputSVG);
        if (!stream) {
            std::cerr << "genalgo: Failed to open file " << globalCfg.outputSVG << std::endl;
            std::cerr << "         Unable to save SVG!" << std::endl;
        } else {
            bestIndividual.toSVG(stream);
        }
    }
    return 0;
}

GA_NAMESPACE_END

// Entry point
int main(int argc, char* argv[]) {
    genalgo::SignalHandler::setup();
    if (!genalgo::setupConfiguration(argc, argv))
        return 1;

    return genalgo::main();
}
