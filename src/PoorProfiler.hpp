#ifndef GENALGO_POORPROFILER_HPP
#define GENALGO_POORPROFILER_HPP

#include "base.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <chrono>

GA_NAMESPACE_BEGIN

class ProfilerStopwatch {
    using steady_clock = std::chrono::steady_clock;
    using time_point = std::chrono::time_point<steady_clock>;
    using duration = std::chrono::duration<double>;
public:
    std::string_view name() const { return name_; }
    double elapsed() const { return elapsed_; }

    bool parentIs(const ProfilerStopwatch* other) const {
        return parent_ == other;
    }

    void reset() {
        elapsed_ = 0;
    }

private:
    void restart() {
        startTime_ = std::chrono::steady_clock::now();
    }

    void trigger() {
        time_point endTime = steady_clock::now();

        duration elapsed = endTime - startTime_;
        elapsed_ += std::chrono::duration_cast<duration>(elapsed).count();
    }

    ProfilerStopwatch(std::string_view name, ProfilerStopwatch* parent)
        : name_(name), parent_(parent), elapsed_(0) { }

    std::string name_;
    ProfilerStopwatch* parent_;
    time_point startTime_;
    double elapsed_;
    friend class PoorProfiler;
};

class PoorProfiler {
public:
    PoorProfiler() {
        stopwatches.reserve(32);
    }

    void start(const char* name, const char* display) {
        ProfilerStopwatch* sw;
        auto it = stopwatchesMap.find(name);
        if (it == stopwatchesMap.end())
            sw = &newStopwatch(name, display);
        else
            sw = it->second;

        activeStopwatches.push_back(sw);
        sw->restart();
    }

    void start(const char* name) {
        start(name, name);
    }

    void stop(const char* name) {
        auto it = stopwatchesMap.find(name);
        if (it == stopwatchesMap.end())
            throw_bad_stopwatch();
        ProfilerStopwatch* sw = it->second;
        if (activeStopwatches.empty() || activeStopwatches.back() != sw)
            throw_bad_pop(name);
        activeStopwatches.pop_back();
        sw->trigger();
    }

    ProfilerStopwatch& getStopwatch(const char* name) {
        auto it = stopwatchesMap.find(name);
        if (it == stopwatchesMap.end())
            throw_bad_stopwatch();
        return *it->second;
    }

    std::vector<ProfilerStopwatch> const& getStopwatches() const {
        return stopwatches;
    }

    std::vector<ProfilerStopwatch>& getStopwatches() {
        return stopwatches;
    }
private:
    std::vector<ProfilerStopwatch*> activeStopwatches;
    std::unordered_map<const char*, ProfilerStopwatch*> stopwatchesMap;
    std::vector<ProfilerStopwatch> stopwatches;

    ProfilerStopwatch& newStopwatch(const char* name, const char* display);
    [[noreturn]] void throw_bad_stopwatch();
    [[noreturn]] void throw_bad_pop(const char* name);
};

extern PoorProfiler profiler;

class ProfilerGuard {
public:
    ProfilerGuard(PoorProfiler& prof, const char* name, const char* display)
        : name(name), prof(prof) {
        prof.start(name, display);
    }
    ProfilerGuard(PoorProfiler& prof, const char* name)
        : ProfilerGuard(prof, name, name) { }

    ProfilerGuard(const char* name, const char* display)
        : ProfilerGuard(profiler, name, display) { }
    ProfilerGuard(const char* name)
        : ProfilerGuard(profiler, name, name) { }

    ~ProfilerGuard() {
        prof.stop(name);
    }
private:
    const char* name;
    PoorProfiler& prof;
};

GA_NAMESPACE_END

#endif // GENALGO_POORPROFILER_HPP
