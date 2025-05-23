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

struct VecTriangle {
    Vec2f a, b, c;
    Color color;
};

struct OptimizedVecTriangle {
    Vec2f a, b, c;
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
    std::unique_ptr<IndividualInfo[]> hostIndividualInfo = nullptr;

    f64* deviceFitnesses = nullptr;
    IndividualInfo* deviceIndividualInfo = nullptr;
    Vec3f* deviceCanvas = nullptr;
    Vec3f* deviceImage = nullptr;
    f64* deviceWeights = nullptr;

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
        std::fprintf(stderr, "CudaFitnessEngine: canvas size is too large\n");
        std::fprintf(stderr, "  This is a limitation of the current implementation\n");
        std::abort();
    }

    triangles.reserve(populationSize * 100);
    fitnesses.resize(populationSize);

    hostIndividualInfo = std::make_unique<IndividualInfo[]>(populationSize);

    deviceMalloc(&deviceFitnesses, populationSize);
    deviceMalloc(&deviceCanvas, canvasSize);
    deviceMalloc(&deviceImage, imSize);
    deviceMalloc(&deviceWeights, imSize);
    deviceMalloc(&deviceIndividualInfo, populationSize);

    auto hostImage = std::make_unique<Vec3f[]>(imSize);
    auto hostWeights = std::make_unique<f64[]>(imSize);

    Color* target = reinterpret_cast<Color*>(globalCfg.targetImage.getData());
    f64* weights = globalCfg.targetImage.getWeights();
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
                    Vec3f rgb = (target[i].a / 255.0f) * fromColor(target[i]);

                    hostImage[j] = rgb;
                    hostWeights[j] = weights[i];
                    ++j;
                }
            }
        }
    }
    if (j != imSize) {
        std::fprintf(stderr, "CudaFitnessEngine: j != imSize\n");
        std::abort();
    }

    copyHostToDevice(deviceImage, hostImage.get(), imSize);
    copyHostToDevice(deviceWeights, hostWeights.get(), imSize);

    // Shared memory is now fixed and doesn't depend on maxTriangles anymore :)

    // Limit of shared memory per block is 49152 bytes
    // i32 hardLimitTriangles = 49100 / sizeof(OptimizedVecTriangle);
    // if (globalCfg.maxTriangles > hardLimitTriangles) {
    //     std::fprintf(stderr, "CudaFitnessEngine: maxTriangles is too large: %d\n", globalCfg.maxTriangles);
    //     std::fprintf(stderr, "  The current implementation can only handle %d triangles\n", hardLimitTriangles);
    //     std::abort();
    // }
}

CudaFitnessEngine::Engine::~Engine() {
    cudaFree(deviceFitnesses);
    cudaFree(deviceCanvas);
    cudaFree(deviceImage);
    cudaFree(deviceIndividualInfo);
    cudaFree(deviceWeights);
}

inline static __device__
bool pointInTriangle(Vec2f p, Vec2f a, Vec2f b, Vec2f c) {
    auto sign = [](Vec2f p1, Vec2f p2, Vec2f p3) {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    };

    bool b1, b2, b3;
    b1 = sign(p, a, b) < 0;
    b2 = sign(p, b, c) < 0;
    b3 = sign(p, c, a) < 0;

    return ((b1 == b2) && (b2 == b3));
}

inline static __device__
bool pointInBox(Vec2f p, Vec2f a, Vec2f b) {
    return p.x >= a.x && p.x <= b.x && p.y >= a.y && p.y <= b.y;
}

inline static __device__
Vec3f colorBlend(Vec3f dst, Vec3f src, f32 alpha) {
    return dst * (1.0f - alpha) + src * alpha;
}

[[maybe_unused]] inline static __device__
bool isTriangleInBounds0(VecTriangle const& t, f32 x, f32 y) {
    f32 x2 = x + 16;
    f32 y2 = y + 16;

    f32 minX = min(t.a.x, min(t.b.x, t.c.x));
    f32 minY = min(t.a.y, min(t.b.y, t.c.y));
    f32 maxX = max(t.a.x, max(t.b.x, t.c.x));
    f32 maxY = max(t.a.y, max(t.b.y, t.c.y));

    bool bad = false;
    bad |= (minX >= x2);
    bad |= (maxX < x);
    bad |= (minY >= y2);
    bad |= (maxY < y);

    return !bad;
}

inline static __device__
bool doesSegmentIntersect(Vec2f a, Vec2f b, Vec2f c, Vec2f d) {
    auto cross = [](Vec2f v, Vec2f w) {
        return v.x * w.y - v.y * w.x;
    };

    auto isBetween = [](float a, float b, float c) {
        return fmin(a, b) <= c && c <= fmax(a, b);
    };

    Vec2f ab = {b.x - a.x, b.y - a.y};
    Vec2f ac = {c.x - a.x, c.y - a.y};
    Vec2f ad = {d.x - a.x, d.y - a.y};
    Vec2f cd = {d.x - c.x, d.y - c.y};
    Vec2f ca = {a.x - c.x, a.y - c.y};
    Vec2f cb = {b.x - c.x, b.y - c.y};

    float cross1 = cross(ab, ac);
    float cross2 = cross(ab, ad);
    float cross3 = cross(cd, ca);
    float cross4 = cross(cd, cb);

    // Check for intersection using cross products
    bool intersect = (cross1 * cross2 < 0) && (cross3 * cross4 < 0);

    // Check for collinear cases
    bool collinear1 = (cross1 == 0) && isBetween(a.x, b.x, c.x) && isBetween(a.y, b.y, c.y);
    bool collinear2 = (cross2 == 0) && isBetween(a.x, b.x, d.x) && isBetween(a.y, b.y, d.y);
    bool collinear3 = (cross3 == 0) && isBetween(c.x, d.x, a.x) && isBetween(c.y, d.y, a.y);
    bool collinear4 = (cross4 == 0) && isBetween(c.x, d.x, b.x) && isBetween(c.y, d.y, b.y);

    return intersect || collinear1 || collinear2 || collinear3 || collinear4;
}

inline static __device__
bool isTriangleInBounds(VecTriangle const& t, f32 x, f32 y) {
    f32 x2 = x + 16;
    f32 y2 = y + 16;

    Vec2f a {x, y};
    Vec2f b {x2, y};
    Vec2f c {x, y2};
    Vec2f d {x2, y2};

    if (pointInTriangle(a, t.a, t.b, t.c))
        return true;

    if (pointInBox(t.a, a, d))
        return true;

    Vec2f triangleSegments[3][2] = {
        {t.a, t.b},
        {t.b, t.c},
        {t.c, t.a}
    };

    Vec2f boxSegments[4][2] = {
        {a, b},
        {b, d},
        {d, c},
        {c, a}
    };

    for (i32 i = 0; i < 3; ++i) {
        for (i32 j = 0; j < 4; ++j) {
            if (doesSegmentIntersect(triangleSegments[i][0], triangleSegments[i][1],
                                     boxSegments[j][0], boxSegments[j][1]))
                return true;
        }
    }

    return false;
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

constexpr i32 MAX_SHARED_TRIANGLES = 256;

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

    // __shared__ VecTriangle preSharedTriangles[MAX_SHARED_TRIANGLES];
    __shared__ OptimizedVecTriangle sharedTriangles[MAX_SHARED_TRIANGLES];
    __shared__ i32 numTrianglesShared;

    f32 tX = 16 * tileX;
    f32 tY = 16 * tileY;
    Vec3f pixel = Vec3f{0.0, 0.0, 0.0};

    i32 x = tileX * 16 + threadIdx.x % 16;
    i32 y = tileY * 16 + threadIdx.x / 16;

    i32 numProcessedTriangles = 0;
    while (numProcessedTriangles < numTriangles) {
        i32 numItTriangles = min(numTriangles - numProcessedTriangles, MAX_SHARED_TRIANGLES);

        // I've tried to coalesce the memory load, but for some reason it's slower
        // __syncthreads();
        // {
        //     u32* triangles_ = reinterpret_cast<u32*>(triangles + numProcessedTriangles);
        //     u32* preSharedTriangles_ = reinterpret_cast<u32*>(preSharedTriangles);

        //     static_assert(sizeof(VecTriangle) % sizeof(u32) == 0);
        //     i32 numBytes = (numItTriangles * sizeof(VecTriangle)) / sizeof(u32);

        //     for (i32 j = threadIdx.x; j < numBytes; j += blockDim.x) {
        //         u32 value = triangles_[j];
        //         preSharedTriangles_[j] = value;
        //     }

        //     // for (i32 j = threadIdx.x; j < numItTriangles; j += blockDim.x) {
        //     //     preSharedTriangles[j] = triangles[j];
        //     // }
        // }

        __syncthreads();
        for (i32 j = threadIdx.x; j < numItTriangles; j += blockDim.x) {
            VecTriangle const& triangle = triangles[numProcessedTriangles + j];
            // VecTriangle const& triangle = preSharedTriangles[j];

            sharedTriangles[j].a = triangle.a;
            sharedTriangles[j].b = triangle.b;
            sharedTriangles[j].c = triangle.c;
            sharedTriangles[j].color = fromColor(triangle.color);
            sharedTriangles[j].alpha = triangle.color.a / 255.0f;

            bool bounds = isTriangleInBounds(triangle, tX, tY);
            sharedTriangles[j].use = bounds;
        }
        numProcessedTriangles += numItTriangles;

        constexpr i32 N = 32;
        __shared__ i32 mShared[N];

        __syncthreads();
        if (threadIdx.x < N) {
            i32 id = threadIdx.x;

            i32 ini = id * numItTriangles / N;
            i32 fim = (id + 1) * numItTriangles / N;
            fim = min(fim, numItTriangles);
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

        if (x < width && y < height) {
            Vec2f p = Vec2f{static_cast<f32>(x), static_cast<f32>(y)};

            for (i32 j = 0; j < numTrianglesShared; ++j) {
                OptimizedVecTriangle const& t = sharedTriangles[j];

                if (pointInTriangle(p, t.a, t.b, t.c)) {
                    pixel = colorBlend(pixel, t.color, t.alpha);
                }
            }
        }
    }

    if (x >= width || y >= height)
        return;

    i32 imSize = image.size;
    Vec3f* canvas = image.canvas + i * imSize;

    // i32 idx = threadIdx.x;

    // i32 dy = idx / 16;
    // i32 dx = idx % 15;

    // i32 rowH = min(16, height - tileY*16);
    // i32 colW = min(16, width - tileX*16);

    // i32 pixelsBeforeRows = min(tileY*16, height) * width;

    // i32 pixelsBeforeTiles = rowH * min(tileX*16, width);

    // i32 offset = dy * colW + dx;

    // i32 xy = pixelsBeforeRows + pixelsBeforeTiles + offset;
    i32 m = 16;
    if (tileY == image.height / 16)
        m = image.height % 16;

    i32 xy = 16 * (tileY * width + m * tileX) + threadIdx.x;

    canvas[xy] = pixel;
}

// Number of threads per individual in the computeFitnessKernel
// It's better if it's a multiple of 32
static __global__
void computeFitnessKernel(
        Vec3f* target, f64* weights, Vec3f* canvas_, 
        f64* fitness, i32 imWidth, i32 imHeight, i32 populationSize) {

    i32 i = blockIdx.x;
    i32 nThreads = blockDim.x;
    i32 j = threadIdx.x;

    i32 imSize = imWidth * imHeight;
    i32 numTasks = imSize;

    i32 task0 = j * (numTasks / nThreads) + min(j, numTasks % nThreads);
    i32 n = numTasks / nThreads + (j < numTasks % nThreads);

    Vec3f* canvas = canvas_ + i * imSize;

    f64 fitnessSum = 0.0;
    for (i32 xy = task0; xy < task0 + n; ++xy) {
        Vec3f imgPixel = target[xy];
        Vec3f pixel = canvas[xy];

        double dx = imgPixel.x - pixel.x;
        double dy = imgPixel.y - pixel.y;
        double dz = imgPixel.z - pixel.z;

        double sum = dx * dx + dy * dy + dz * dz;
        fitnessSum += sum * (1.0 + weights[xy]);
    }

    extern __shared__ f64 fitnessesSums[];
    fitnessesSums[j] = fitnessSum;

    __syncthreads();
    if (j == 0) { // Reduction in a single warp (not optimal)
        f64 totalFitness = 0.0;
        for (i32 k = 0; k < nThreads; ++k) {
            totalFitness += fitnessesSums[k];
        }
        fitness[i] = totalFitness;
    }
}

void CudaFitnessEngine::Engine::evaluate(std::vector<Individual>& individuals) {
    if (individuals.size() != populationSize) {
        std::fprintf(stderr, "CudaFitnessEngine::evaluate: individuals.size() != populationSize\n");
        std::abort();
    }

    defer { profiler.stop("cudaFitness:cleanup"); };

    profiler.start("cudaFitness:prepare", "Prepare");
    i32 maxTriangles = 0;
    triangles.clear();
    for (i32 i = 0; i < populationSize; ++i) {
        Individual const& ind = individuals[i];
        maxTriangles = std::max(maxTriangles, ind.size());

        hostIndividualInfo[i].offset = triangles.size();
        for (Triangle const& t : ind) {
            VecTriangle vt;
            vt.a = Vec2f(t.a.x, t.a.y);
            vt.b = Vec2f(t.b.x, t.b.y);
            vt.c = Vec2f(t.c.x, t.c.y);
            vt.color = t.color;

            triangles.push_back(vt);
        }

        hostIndividualInfo[i].size = ind.size();

        // To ensure 32-bytes alignment
        while ((sizeof(VecTriangle) * triangles.size()) % 32 != 0) {
            VecTriangle vt;
            vt.a = Vec2f(0, 0);
            vt.b = Vec2f(0, 0);
            vt.c = Vec2f(0, 0);
            vt.color = Color{0, 0, 0, 0};

            triangles.push_back(vt);
        }
    }
    profiler.stop("cudaFitness:prepare");

    profiler.start("cudaFitness:copy2device", "Copy");
    auto deviceTriangles = deviceMalloc<VecTriangle>(triangles.size());
    defer { cudaFree(deviceTriangles); };

    copyHostToDevice(deviceTriangles, triangles.data(), triangles.size());
    copyHostToDevice(deviceIndividualInfo, hostIndividualInfo.get(), populationSize);
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

        drawTriangles<<<BLOCKS, THREADS>>>(imageInfo, data);
        // drawTriangles<<<BLOCKS, THREADS,
        //     maxTriangles * sizeof(OptimizedVecTriangle)
        // >>>(imageInfo, data);

        CUDA_CHECK(cudaPeekAtLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
    }
    profiler.stop("cudaFitness:draw");

    profiler.start("cudaFitness:compute", "Compute");
    {
        i32 N = populationSize;
        u32 THREADS = 32;
        u32 BLOCKS = N;
        cudaMemset(deviceFitnesses, 0, populationSize * sizeof(*deviceFitnesses));
        computeFitnessKernel<<<BLOCKS, THREADS, THREADS * sizeof(f64)>>>(
                deviceImage, deviceWeights, deviceCanvas, deviceFitnesses, imWidth, imHeight, populationSize);
        CUDA_CHECK(cudaPeekAtLastError());
        copyDeviceToHost(fitnesses.data(), deviceFitnesses, populationSize);
        cudaDeviceSynchronize();
    }
    profiler.stop("cudaFitness:compute");

    profiler.start("cudaFitness:copy2individuals", "Copy to individuals");
    for (i32 i = 0; i < populationSize; ++i) {
        individuals[i].setFitness(std::pow(fitnesses[i], 0.7));
    }
    profiler.stop("cudaFitness:copy2individuals");

    profiler.start("cudaFitness:cleanup", "Cleanup");
}

// Wrapper for the actual implementation of the engine

CudaFitnessEngine::CudaFitnessEngine() :
    impl(std::make_unique<Engine>()) {}

CudaFitnessEngine::~CudaFitnessEngine() = default;

void CudaFitnessEngine::evaluate_impl(std::vector<Individual>& individuals) {
    impl->evaluate(individuals);
}

GA_NAMESPACE_END
