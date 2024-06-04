#include "CudaFitnessEngine.hpp"

#include <cstdio>
#include <cstdlib>
#include "PoorProfiler.hpp"
#include "Vec.hpp"
#include "defer.hpp"
#include "globalConfig.hpp"

static void cudaCheck(cudaError_t error, const char* message) {
    if (error != cudaSuccess) {
        std::fprintf(stderr, "CUDA error: %s: %s\n", message, cudaGetErrorString(error));
        std::abort();
    }
}

#define CUDA_CHECK(expr) ::cudaCheck(expr, #expr)

GA_NAMESPACE_BEGIN

constexpr i32 THREADS_PER_INDIVIDUAL = 32;

struct VecTriangle {
    Vec2i a, b, c;
    Color color;
};
static_assert(sizeof(VecTriangle) == sizeof(Triangle) &&
              alignof(VecTriangle) == alignof(Triangle), "VecTriangle is not layout-compatible with Triangle");

template<typename T>
inline void deviceMalloc(T** ptr, size_t count) {
    CUDA_CHECK(cudaMalloc(ptr, count * sizeof(T)));
}

template<typename T>
inline T* deviceMalloc(size_t count) {
    T* ptr;
    deviceMalloc(&ptr, count);
    return ptr;
}

static __host__ __device__
Vec3f fromColor(Color c) {
    return Vec3f{1.0f * c.r, 1.0f * c.g, 1.0f * c.b};
}

class CudaFitnessEngine::Engine {
public:
    Engine();
    ~Engine();

    void evaluate(std::vector<Individual>& individuals);
private:
    std::vector<VecTriangle> triangles;
    std::vector<f64> fitnesses;
    Vec2i* hostNumTriangles = nullptr;

    f64* deviceFitnesses = nullptr;
    Vec2i* deviceNumTriangles = nullptr;
    Vec3f* deviceCanvas = nullptr;
    Vec3f* deviceImage = nullptr;

    i32 imWidth, imHeight, imSize;
    i64 canvasSize;
    i32 populationSize;
};

CudaFitnessEngine::Engine::Engine() {
    populationSize = globalCfg.populationSize;
    imWidth = globalCfg.targetImage.getWidth();
    imHeight = globalCfg.targetImage.getHeight();
    imSize = imWidth * imHeight;
    canvasSize = static_cast<i64>(imSize) * populationSize;
    if (canvasSize > std::numeric_limits<i32>::max()) {
        // FIXME: Implement support for 64-bits canvas
        std::fprintf(stderr, "CudaFitnessEngine: canvas size is too large\n");
        std::fprintf(stderr, "  This is a limitation of the current implementation, for now :)\n");
        std::abort();
    }

    triangles.reserve(populationSize * 100);
    fitnesses.resize(populationSize);

    hostNumTriangles = new Vec2i[populationSize];

    deviceMalloc(&deviceFitnesses, populationSize);
    deviceMalloc(&deviceCanvas, canvasSize);
    deviceMalloc(&deviceImage, imSize);
    deviceMalloc(&deviceNumTriangles, populationSize);

    Vec3f* hostImage = new Vec3f[imSize];
    defer { delete[] hostImage; };

    Color* target = reinterpret_cast<Color*>(globalCfg.targetImage.getData());
    for (i32 y = 0; y < imHeight; ++y) {
        for (i32 x = 0; x < imWidth; ++x) {
            i32 i = y * imWidth + x;
            hostImage[i] = (target[i].a / 255.0f) * fromColor(target[i]);
        }
    }

    CUDA_CHECK(cudaMemcpy(deviceImage, hostImage, imSize * sizeof(Vec3f), cudaMemcpyHostToDevice));
}

CudaFitnessEngine::Engine::~Engine() {
    cudaFree(deviceFitnesses);
    cudaFree(deviceCanvas);
    cudaFree(deviceImage);
    cudaFree(deviceNumTriangles);

    free(hostNumTriangles);
}

static __device__
bool pointInTriangle(Vec2i p, Vec2i a, Vec2i b, Vec2i c) {
    auto sign = [](Vec2i p1, Vec2i p2, Vec2i p3) {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    };

    bool b1, b2, b3;
    b1 = sign(p, a, b) < 0;
    b2 = sign(p, b, c) < 0;
    b3 = sign(p, c, a) < 0;

    return ((b1 == b2) && (b2 == b3));
}

inline static __device__
Vec3f colorBlend(Vec3f dst, Color src) {
    float alpha = src.a / 255.0;

    return dst * (1.0f - alpha) + fromColor(src) * alpha;
}

[[maybe_unused]] static __global__
void drawTriangles(Vec3f* allCanvas, VecTriangle* allTriangles, Vec2i* allNumTriangles, i32 width, i32 height, i32 populationSize) {
    i32 size = width * height;

    i32 xy = blockIdx.x * blockDim.x + threadIdx.x;
    i32 i = blockIdx.y;
    if (xy >= size)
        return;

    Vec3f* canvas = allCanvas + i * size;
    VecTriangle* triangles = allTriangles + allNumTriangles[i].x;
    i32 numTriangles = allNumTriangles[i].y;

    extern __shared__ VecTriangle sharedTriangles[];
    for (i32 j = threadIdx.x; j < numTriangles; j += blockDim.x) {
        sharedTriangles[j] = triangles[j];
    }
    __syncthreads();

    Vec3f pixel = Vec3f{0.0, 0.0, 0.0};

    i32 x = xy % width;
    i32 y = xy / width;

    for (i32 j = 0; j < numTriangles; ++j) {
        VecTriangle const& t = sharedTriangles[j];
        if (pointInTriangle(Vec2i{x, y}, t.a, t.b, t.c)) {
            pixel = colorBlend(pixel, t.color);
        }
    }
    canvas[xy] = pixel;
}

static __global__
void computeFitnessKernel(
        Vec3f* target, Vec3f* canvas, 
        f64* fitness, i32 imWidth, i32 imHeight, i32 populationSize) {
    i32 numTasks = imWidth * imHeight;
    i32 threadId = blockDim.x * blockIdx.x + threadIdx.x;
    i32 i = threadId / THREADS_PER_INDIVIDUAL;
    if (i >= populationSize)
        return;
    i32 j = threadId % THREADS_PER_INDIVIDUAL;

    f64 fitnessSum = 0.0;
    i32 task0 = j * (numTasks / THREADS_PER_INDIVIDUAL) + min(j, numTasks % THREADS_PER_INDIVIDUAL);
    i32 n = numTasks / THREADS_PER_INDIVIDUAL + (j < numTasks % THREADS_PER_INDIVIDUAL);

    i32 imSize = imWidth * imHeight;
    for (i32 xy = task0; xy < task0 + n; ++xy) {
        Vec3f imgPixel = target[xy];
        Vec3f pixel = canvas[i * imSize + xy];

        double dx = imgPixel.x - pixel.x;
        double dy = imgPixel.y - pixel.y;
        double dz = imgPixel.z - pixel.z;

        fitnessSum += dx * dx + dy * dy + dz * dz;   
    }

    atomicAdd(&fitness[i], fitnessSum);
}

void CudaFitnessEngine::Engine::evaluate(std::vector<Individual>& individuals) {
    if (individuals.size() != populationSize) {
        std::fprintf(stderr, "CudaFitnessEngine::evaluate: individuals.size() != populationSize\n");
        std::abort();
    }

    defer { profiler.stop("cudaFitness:cleanup"); };

    std::vector<VecTriangle> hostTriangles(triangles.size());

    i32 maxTriangles = 0;
    triangles.clear();
    for (i32 i = 0; i < populationSize; ++i) {
        Individual const& ind = individuals[i];
        hostNumTriangles[i].x = triangles.size();
        hostNumTriangles[i].y = ind.size();
        maxTriangles = std::max(maxTriangles, ind.size());
        for (Triangle const& t : ind) {
            VecTriangle vt;
            vt.a = Vec2i(t.a.x, t.a.y);
            vt.b = Vec2i(t.b.x, t.b.y);
            vt.c = Vec2i(t.c.x, t.c.y);
            vt.color = t.color;
            triangles.push_back(vt);
        }
    }

    profiler.start("cudaFitness:copy2device", "Copy");
    auto deviceTriangles = deviceMalloc<VecTriangle>(triangles.size());
    defer { cudaFree(deviceTriangles); };
    cudaMemcpy(deviceTriangles, triangles.data(), triangles.size() * sizeof(VecTriangle), cudaMemcpyHostToDevice);
    cudaMemcpy(deviceNumTriangles, hostNumTriangles, populationSize * sizeof(u32), cudaMemcpyHostToDevice);
    cudaDeviceSynchronize(); // FIXME: Remove this
    profiler.stop("cudaFitness:copy2device");

    profiler.start("cudaFitness:draw", "Draw");
    {
        u32 THREADS = 256;

        dim3 BLOCKS;
        BLOCKS.x = (imSize + THREADS - 1) / THREADS;
        BLOCKS.y = populationSize;
        BLOCKS.z = 1;

        drawTriangles<<<BLOCKS, THREADS,
            maxTriangles * sizeof(VecTriangle)
        >>>(deviceCanvas, deviceTriangles, deviceNumTriangles,
                imWidth, imHeight, populationSize);

        // u32 BLOCKS = (imSize + THREADS - 1) / THREADS;
        // i32 numTriangles = 0;
        // for (i32 i = 0; i < populationSize; ++i) {
        //     drawTriangles<<<BLOCKS, THREADS>>>(deviceCanvas + i * imSize,
        //             deviceTriangles + numTriangles, individuals[i].size(),
        //             imWidth, imHeight, populationSize);
        //     numTriangles += individuals[i].size();
        // }
        cudaDeviceSynchronize(); // FIXME: Remove this
    }
    profiler.stop("cudaFitness:draw");

    profiler.start("cudaFitness:compute", "Compute");
    i32 N = populationSize * THREADS_PER_INDIVIDUAL;
    u32 THREADS = 128;
    u32 BLOCKS = (N + THREADS - 1) / THREADS;
    cudaMemset(deviceFitnesses, 0, populationSize * sizeof(f64));
    computeFitnessKernel<<<BLOCKS, THREADS>>>(
            deviceImage, deviceCanvas, deviceFitnesses, imWidth, imHeight, populationSize);
    CUDA_CHECK(cudaMemcpy(fitnesses.data(), deviceFitnesses, populationSize * sizeof(f64), cudaMemcpyDeviceToHost));
    cudaDeviceSynchronize();
    profiler.stop("cudaFitness:compute");

    profiler.start("cudaFitness:copy2individuals", "Copy to individuals");
    for (i32 i = 0; i < populationSize; ++i) {
        individuals[i].setFitness(fitnesses[i]);
    }
    profiler.stop("cudaFitness:copy2individuals");


    profiler.start("cudaFitness:cleanup", "Cleanup");
}

// Wrapper for the actual implementation of the engine

CudaFitnessEngine::CudaFitnessEngine() :
    impl(std::make_unique<Engine>()) {}

CudaFitnessEngine::~CudaFitnessEngine() = default;

void CudaFitnessEngine::evaluate(std::vector<Individual>& individuals) {
    impl->evaluate(individuals);
}

GA_NAMESPACE_END
