#include "PoorProfiler.hpp"

GA_NAMESPACE_BEGIN

PoorStopwatch sLoop;
PoorStopwatch sEvaluation, sOpenGL, sCuda;
PoorStopwatch sCudaMapping, sCudaTextures, sCudaKernel, sCudaCopy, sCudaCleanup;
PoorStopwatch sRender, sBreed;

GA_NAMESPACE_END
