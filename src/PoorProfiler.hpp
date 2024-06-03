#ifndef GENALGO_POORPROFILER_HPP
#define GENALGO_POORPROFILER_HPP

#include "base.hpp"
#include <ctime>

GA_NAMESPACE_BEGIN

class PoorStopwatch {
public:
    static double getTime() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec + ts.tv_nsec * 1e-9;
    }
    PoorStopwatch() : startTime(0), measuredTime(0) {}

    void restart() {
        startTime = getTime();
    }
    
    void trigger() {
        double endTime = getTime();
        measuredTime += endTime - startTime;
        startTime = endTime;
    }

    double elapsed() const {
        return measuredTime;
    }

    void reset() {
        measuredTime = 0;
    }
private:
    double startTime;
    double measuredTime;
};

extern PoorStopwatch sLoop;
extern PoorStopwatch sEvaluation, sOpenGL, sCuda;
extern PoorStopwatch sCudaMapping, sCudaTextures, sCudaKernel, sCudaCopy, sCudaCleanup;
extern PoorStopwatch sRender, sBreed;

GA_NAMESPACE_END

#endif // GENALGO_POORPROFILER_HPP
