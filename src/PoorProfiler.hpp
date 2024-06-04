#ifndef GENALGO_POORPROFILER_HPP
#define GENALGO_POORPROFILER_HPP

#include "base.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <ctime>

GA_NAMESPACE_BEGIN

class ProfilerStopwatch {
public:
    static double getTime() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec + ts.tv_nsec * 1e-9;
    }

    std::string_view name() const { return name_; }
    double elapsed() const { return elapsed_; }

    bool parentIs(const ProfilerStopwatch* other) const {
        return parent_ == other;
    }

private:
    void restart() {
        startTime_ = getTime();
    }

    void trigger() {
        double endTime = getTime();
        elapsed_ += endTime - startTime_;
    }

    ProfilerStopwatch(std::string_view name, ProfilerStopwatch* parent)
        : name_(name), parent_(parent), elapsed_(0) { }

    std::string name_;
    ProfilerStopwatch* parent_;
    double startTime_, elapsed_;
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
