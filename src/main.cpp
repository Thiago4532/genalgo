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

    std::cout << std::fixed << std::setprecision(2);
    i64 nGen;
    for (nGen = 1; !shouldStop(); ++nGen) {
        profiler.start("loop");

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
            std::cout << "Generation " << nGen << '\n';
            std::cout << "Seed " << globalCfg.seed << '\n';
            
            std::cout << "Best fitness so far: " << bestIndividual.size() << " " <<
                bestIndividual.getFitness() << '\n';

            // i32 total = 0;
            // double maxRatio = 0.0;
            // for (Individual const& i : pop.getIndividuals()) {
            //     i32 amount[4] = {0, 0, 0, 0};
            //     for (Triangle const& t : i) {
            //         i32 minX = std::min({t.a.x, t.b.x, t.c.x});
            //         i32 minY = std::min({t.a.y, t.b.y, t.c.y});
            //         i32 maxX = std::max({t.a.x, t.b.x, t.c.x});
            //         i32 maxY = std::max({t.a.y, t.b.y, t.c.y});

            //         static constexpr auto cross = [](Point<i32> const& a, Point<i32> const& b, Point<i32> const& c) {
            //             return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
            //         };
            //         static constexpr auto transform = [](Point<i32> const& p, bool flipX, bool flipY) {
            //             Point<i32> r;
            //             r.x = flipX ? globalCfg.targetImage.getWidth() - p.x : p.x;
            //             r.y = flipY ? globalCfg.targetImage.getHeight() - p.y : p.y;
            //             return r;
            //         };

            //         static constexpr auto check = [](Triangle const& t, bool flipX, bool flipY) {
            //             Point<i32> a = transform(t.a, flipX, flipY);
            //             Point<i32> b = transform(t.b, flipX, flipY);
            //             Point<i32> c = transform(t.c, flipX, flipY);

            //             i32 halfWidth = (globalCfg.targetImage.getWidth() + 1) / 2;
            //             i32 halfHeight = (globalCfg.targetImage.getHeight() + 1) / 2;

            //             if (a.x <= halfWidth && a.y <= halfHeight)
            //                 return true;
            //             if (b.x <= halfWidth && b.y <= halfHeight)
            //                 return true;
            //             if (c.x <= halfWidth && c.y <= halfHeight)
            //                 return true;

            //             Point minX = a;
            //             if (b.x < minX.x)
            //                 minX = b;
            //             if (c.x < minX.x)
            //                 minX = c;

            //            Point minY = a; 
            //            if (b.y < minY.y)
            //                minY = b;
            //            if (c.y < minY.y)
            //                minY = c;

            //            if (minX.y > halfHeight || minY.x > halfWidth)
            //                return false;

            //            auto C = cross(minX, {halfWidth, halfHeight}, minY);
            //            return (C <= 0);
            //         };

            //         amount[0] += check(t, false, false);
            //         amount[1] += check(t, true, false);
            //         amount[2] += check(t, false, true);
            //         amount[3] += check(t, true, true);
                    
            //         total++;
            //     }

            //     i32 largest = std::max({amount[0], amount[1], amount[2], amount[3]});
            //     double ratio = 100.0 * largest / i.size();
            //     maxRatio = std::max(maxRatio, ratio);
            // }

            // std::cout << "Ratio: " << maxRatio << "%\n";

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

            // if (nGen == 1350) {
            //     std::cout << "Fitness: " << bestIndividual.getFitness() << '\n';
            //     break;
            // }
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
