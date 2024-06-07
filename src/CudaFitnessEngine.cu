#include "CudaFitnessEngine.hpp"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "PoorProfiler.hpp"
#include "Vec.hpp"
#include "defer.hpp"
#include "GlobalConfig.hpp"

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

struct OptimizedVecTriangle {
    Vec2i a, b, c;
    Vec3f color;
    f32 alpha;
    bool use;
};

struct IndividualInfo {
    i32 offset;
    i32 size;
};

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

template<typename T>
inline void copyHostToDevice(T* device, T const* host, size_t count) {
    CUDA_CHECK(cudaMemcpy(device, host, count * sizeof(T), cudaMemcpyHostToDevice));
}

template<typename T>
inline void copyDeviceToHost(T* host, T const* device, size_t count) {
    CUDA_CHECK(cudaMemcpy(host, device, count * sizeof(T), cudaMemcpyDeviceToHost));
}

inline static __host__ __device__
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
    IndividualInfo* hostIndividualInfo = nullptr;

    f64* deviceFitnesses = nullptr;
    IndividualInfo* deviceIndividualInfo = nullptr;
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

    hostIndividualInfo = new IndividualInfo[populationSize];

    deviceMalloc(&deviceFitnesses, populationSize);
    deviceMalloc(&deviceCanvas, canvasSize);
    deviceMalloc(&deviceImage, imSize);
    deviceMalloc(&deviceIndividualInfo, populationSize);

    Vec3f* hostImage = new Vec3f[imSize];
    defer { delete[] hostImage; };

    Color* target = reinterpret_cast<Color*>(globalCfg.targetImage.getData());
    i32 j = 0;

    for (i32 tileY = 0; tileY < (imHeight + 15)/16; ++tileY) {
        for (i32 tileX = 0; tileX < (imWidth + 15)/16; ++tileX) {
            for (i32 dy = 0; dy < 16; dy++) {
                for (i32 dx = 0; dx < 16; dx++) {
                    i32 y = tileY * 16 + dy;
                    i32 x = tileX * 16 + dx;
                    if (x >= imWidth || y >= imHeight)
                        continue;

                    i32 i = y * imWidth + x;
                    hostImage[j++] = (target[i].a / 255.0f) * fromColor(target[i]);
                }
            }
        }
    }
    if (j != imSize) {
        std::fprintf(stderr, "CudaFitnessEngine: j != imSize\n");
        std::abort();
    }

    copyHostToDevice(deviceImage, hostImage, imSize);

    // Limit of shared memory per block is 49152 bytes
    if (globalCfg.maxTriangles * sizeof(VecTriangle) > 49100) {
        std::fprintf(stderr, "CudaFitnessEngine: maxTriangles is too large\n");
        std::abort();
    }
}

CudaFitnessEngine::Engine::~Engine() {
    cudaFree(deviceFitnesses);
    cudaFree(deviceCanvas);
    cudaFree(deviceImage);
    cudaFree(deviceIndividualInfo);

    free(hostIndividualInfo);
}

inline static __device__
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

// inline static __device__
// Vec3f colorBlend(Vec3f dst, Color src) {
//     float alpha = src.a / 255.0;

//     return dst * (1.0f - alpha) + fromColor(src) * alpha;
// }

inline static __device__
Vec3f colorBlend(Vec3f dst, Vec3f src, f32 alpha) {
    return dst * (1.0f - alpha) + src * alpha;
}

inline static __device__
bool isTriangleInBounds(VecTriangle const& t, i32 x, i32 y) {
    i32 x2 = x + 16;
    i32 y2 = y + 16;

    i32 minX = min(t.a.x, min(t.b.x, t.c.x));
    i32 minY = min(t.a.y, min(t.b.y, t.c.y));
    i32 maxX = max(t.a.x, max(t.b.x, t.c.x));
    i32 maxY = max(t.a.y, max(t.b.y, t.c.y));

    bool bad = false;
    bad |= (minX >= x2);
    bad |= (maxX < x);
    bad |= (minY >= y2);
    bad |= (maxY < y);

    return !bad;
}

struct GPUImageInfo {
    Vec3f* canvas;
    i32 width, height;
    i32 size;
};

struct GPUDrawData {
    VecTriangle* triangles;
    IndividualInfo* info;
};

#if 1

static __global__
void drawTriangles(GPUImageInfo image, GPUDrawData data) {
    i32 width = image.width;
    i32 height = image.height;

    i32 tileX = blockIdx.x;
    i32 tileY = blockIdx.y;
    i32 i = blockIdx.z;

    VecTriangle* triangles;
    i32 numTriangles;

    triangles = data.triangles + data.info[i].offset;
    numTriangles = data.info[i].size;

    extern __shared__ OptimizedVecTriangle sharedTriangles[];
    __shared__ i32 numTrianglesShared;

    for (i32 j = threadIdx.x; j < numTriangles; j += blockDim.x) {
        VecTriangle const& triangle = triangles[j];
        sharedTriangles[j].a = triangle.a;
        sharedTriangles[j].b = triangle.b;
        sharedTriangles[j].c = triangle.c;
        sharedTriangles[j].color = fromColor(triangle.color);
        sharedTriangles[j].alpha = triangle.color.a / 255.0f;

        bool bounds = isTriangleInBounds(triangle, 16 * tileX, 16 * tileY);
        sharedTriangles[j].use = bounds;
    }

    constexpr i32 N = 32;

    __shared__ i32 mShared[N];
    __syncthreads();
    if (threadIdx.x < N) {
        i32 id = threadIdx.x;

        i32 ini = id * numTriangles / N;
        i32 fim = (id + 1) * numTriangles / N;
        fim = min(fim, numTriangles);
        OptimizedVecTriangle localTriangles[64];

        i32 m = 0;
        for (i32 j = ini; j < fim; j++) {
            OptimizedVecTriangle const& triangle = sharedTriangles[j];
            if (!triangle.use)
                continue;
            localTriangles[m++] = triangle;
        }
        mShared[id] = m;
        __syncwarp();
        if (threadIdx.x == 0) {
            for (i32 j = 1; j < N; j++)
                mShared[j] += mShared[j - 1];
            numTrianglesShared = mShared[N - 1];
        }
        __syncwarp();

        i32 m0 = id == 0 ? 0 : mShared[id - 1];
        for (i32 j = 0; j < m; j++) {
            sharedTriangles[m0 + j] = localTriangles[j];
        }
    }
    __syncthreads();

    i32 x = tileX * 16 + threadIdx.x % 16;
    i32 y = tileY * 16 + threadIdx.x / 16;

    if (x >= width || y >= height)
        return;

    Vec3f pixel = Vec3f{0.0, 0.0, 0.0};

    for (i32 j = 0; j < numTrianglesShared; ++j) {
        OptimizedVecTriangle const& t = sharedTriangles[j];

        if (pointInTriangle(Vec2i{x, y}, t.a, t.b, t.c)) {
            pixel = colorBlend(pixel, t.color, t.alpha);
        }
    }

    Vec3f* canvas = image.canvas + i * image.size;

    i32 m = 16;
    if (tileY == image.height / 16)
        m = image.height % 16;

    i32 xy = 16 * (tileY * width + m * tileX) + threadIdx.x;
    canvas[xy] = pixel;
}

#else

static __global__
void drawTriangles(GPUImageInfo image, GPUDrawData data) {
    i32 width = image.width;
    i32 height = image.height;

    i32 tileX = blockIdx.x;
    i32 tileY = blockIdx.y;
    i32 i = blockIdx.z;

    VecTriangle* triangles;
    i32 numTriangles;

    triangles = data.triangles + data.info[i].offset;
    numTriangles = data.info[i].size;

    extern __shared__ OptimizedVecTriangle sharedTriangles[];
    __shared__ i32 numTrianglesShared;

    for (i32 j = threadIdx.x; j < numTriangles; j += blockDim.x) {
        VecTriangle const& t = triangles[j];
        sharedTriangles[j].a = t.a;
        sharedTriangles[j].b = t.b;
        sharedTriangles[j].c = t.c;
        sharedTriangles[j].color = fromColor(t.color);
        sharedTriangles[j].alpha = t.color.a / 255.0f;

        bool bounds = isTriangleInBounds(t, 16 * tileX, 16 * tileY);
        sharedTriangles[j].use = bounds;
    }
    __syncthreads();
    if (threadIdx.x == 0) {
        i32 m = 0;
        for (i32 j = 0; j < numTriangles; j++) {
            if (!sharedTriangles[j].use)
                continue;

            i32 idx = m++;
            OptimizedVecTriangle const& triangle = sharedTriangles[j];
            sharedTriangles[idx] = triangle;
        }

        numTrianglesShared = m;
    }
    __syncthreads();

    i32 x = tileX * 16 + threadIdx.x % 16;
    i32 y = tileY * 16 + threadIdx.x / 16;

    if (x >= width || y >= height)
        return;

    Vec3f pixel = Vec3f{0.0, 0.0, 0.0};

    for (i32 j = 0; j < numTrianglesShared; ++j) {
        OptimizedVecTriangle const& t = sharedTriangles[j];

        if (pointInTriangle(Vec2i{x, y}, t.a, t.b, t.c)) {
            pixel = colorBlend(pixel, t.color, t.alpha);
        }
    }

    Vec3f* canvas = image.canvas + i * image.size;

    i32 m = 16;
    if (tileY == image.height / 16)
        m = image.height % 16;

    i32 xy = 16 * (tileY * width + m * tileX) + threadIdx.x;
    canvas[xy] = pixel;
}

#endif

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

    // FIXME: Remove this
    // static i32 generation = 0;
    // i64 total = 0;
    // i64 amount_l = 0, amount_r = 0;

    profiler.start("cudaFitness:prepare", "Prepare");
    i32 maxTriangles = 0;
    triangles.clear();
    for (i32 i = 0; i < populationSize; ++i) {
        Individual const& ind = individuals[i];
        maxTriangles = std::max(maxTriangles, ind.size());

        hostIndividualInfo[i].offset = triangles.size();
        for (Triangle const& t : ind) {
            VecTriangle vt;
            vt.a = Vec2i(t.a.x, t.a.y);
            vt.b = Vec2i(t.b.x, t.b.y);
            vt.c = Vec2i(t.c.x, t.c.y);
            vt.color = t.color;

            triangles.push_back(vt);
        }

        hostIndividualInfo[i].size = ind.size();
    }
    profiler.stop("cudaFitness:prepare");

    // generation++;
    // total = triangles.size();
    // if (generation % 10 == 0) {
    //     double ratio = 100 * (amount_l + amount_r) / (double)total;
    //     std::fprintf(stderr, "Generation %d: %.2f%%\n", generation, ratio);
    // }

    profiler.start("cudaFitness:copy2device", "Copy");
    auto deviceTriangles = deviceMalloc<VecTriangle>(triangles.size());
    defer { cudaFree(deviceTriangles); };
    copyHostToDevice(deviceTriangles, triangles.data(), triangles.size());
    copyHostToDevice(deviceIndividualInfo, hostIndividualInfo, populationSize);
    cudaDeviceSynchronize();
    profiler.stop("cudaFitness:copy2device");

    profiler.start("cudaFitness:draw", "Draw");
    {
        // Must be 256 always
        constexpr u32 THREADS = 256;

        dim3 BLOCKS;
        BLOCKS.x = (imWidth + 15) / 16;
        BLOCKS.y = (imHeight + 15) / 16;
        BLOCKS.z = populationSize;

        GPUImageInfo imageInfo {
            .canvas = deviceCanvas,
            .width = imWidth,
            .height = imHeight,
            .size = imSize
        };

        GPUDrawData data {
            .triangles = deviceTriangles,
            .info = deviceIndividualInfo
        };

        drawTriangles<<<BLOCKS, THREADS,
            maxTriangles * sizeof(OptimizedVecTriangle)
        >>>(imageInfo, data);

        CUDA_CHECK(cudaPeekAtLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
    }
    profiler.stop("cudaFitness:draw");

    profiler.start("cudaFitness:compute", "Compute");
    i32 N = populationSize * THREADS_PER_INDIVIDUAL;
    u32 THREADS = 128;
    u32 BLOCKS = (N + THREADS - 1) / THREADS;
    cudaMemset(deviceFitnesses, 0, populationSize * sizeof(*deviceFitnesses));
    computeFitnessKernel<<<BLOCKS, THREADS>>>(
            deviceImage, deviceCanvas, deviceFitnesses, imWidth, imHeight, populationSize);
    CUDA_CHECK(cudaPeekAtLastError());
    copyDeviceToHost(fitnesses.data(), deviceFitnesses, populationSize);
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
