#include "PoorProfiler.hpp"
#include <stdexcept>

GA_NAMESPACE_BEGIN

PoorProfiler profiler;

void PoorProfiler::throw_bad_stopwatch() {
    throw std::runtime_error("Stopwatch: No such name");
}

void PoorProfiler::throw_bad_pop(const char* name) {
    throw std::runtime_error("Stopwatch: Out of order pop for " + std::string(name));
}

ProfilerStopwatch& PoorProfiler::newStopwatch(const char* name, const char* display) {
    // Prevent iterator invalidation
    if (stopwatches.size() == stopwatches.capacity())
        throw std::runtime_error("Stopwatch: Too many stopwatches");

    ProfilerStopwatch sw(display, activeStopwatches.empty() ? nullptr : activeStopwatches.back());
    stopwatches.push_back(sw);
    ProfilerStopwatch* ptr = &stopwatches.back();
    stopwatchesMap[name] = ptr;
    return *ptr;
}

GA_NAMESPACE_END
