#include "Color.hpp"
#include "CudaGLHelper.hpp"

#include <cstdio>
#include <cstdlib>
#include <cuda_gl_interop.h>
#include "Vec.hpp"
#include "globalConfig.hpp"
#include "defer.hpp"

#define CUDA_CHECK(expr) do { \
    cudaError_t error = (expr); \
    if (error != cudaSuccess) { \
        std::fprintf(stderr, "CUDA %s: %s\n", #expr, cudaGetErrorString(error)); \
        std::abort(); \
    } \
} while (0)

GA_NAMESPACE_BEGIN

static Vec3d fromColor(Color c) {
    return Vec3d{1.0 * c.r, 1.0 * c.g, 1.0 * c.b};
}

CudaGLHelper::CudaGLHelper() {
    width = globalCfg.targetImage.getWidth();
    height = globalCfg.targetImage.getHeight();
 
    Vec3d* imageInHost = new Vec3d[width * height];
    defer { delete[] imageInHost; };

    Color* target = reinterpret_cast<Color*>(globalCfg.targetImage.getData());
    for (i32 y = 0; y < height; ++y) {
        for (i32 x = 0; x < width; ++x) {
            i32 i = y * width + x;
            imageInHost[i] = (target[i].a / 255.0) * fromColor(target[i]);
        }
    }

    imageInDevice = nullptr;
    fitnessInDevice = nullptr;

    CUDA_CHECK(cudaMalloc(&imageInDevice, width * height * sizeof(Vec3d)));
    CUDA_CHECK(cudaMemcpy(imageInDevice, imageInHost, width * height * sizeof(Vec3d), cudaMemcpyHostToDevice));

    CUDA_CHECK(cudaMalloc(&fitnessInDevice, globalCfg.populationSize * sizeof(f64)));
}

CudaGLHelper::~CudaGLHelper() {
    cudaFree(imageInDevice);
    cudaFree(fitnessInDevice);
    unregisterTextures();
}

void CudaGLHelper::registerTextures(i32 count, u32 textures[]) {
    resources.resize(count);
    for (i32 i = 0; i < count; ++i)
        CUDA_CHECK(cudaGraphicsGLRegisterImage(&resources[i], textures[i], GL_TEXTURE_2D, cudaGraphicsRegisterFlagsReadOnly));
}

__global__ void computeFitnessKernel(
        Vec3d* target,
        cudaTextureObject_t* textures,
        f64* fitness, i32 width, i32 height) {

    i32 i = threadIdx.x;
    f64 fitnessSum = 0.0;

    for (i32 y = 0; y < height; ++y) {
        for (i32 x = 0; x < width; ++x) {
            uchar4 color = tex2D<uchar4>(textures[i], x, y);
            double alpha = color.w / 255.0;

            Vec3d imgPixel = target[y * width + x];
            double3 pixel = make_double3(alpha * color.x, alpha * color.y, alpha * color.z);

            double dx = imgPixel.x - pixel.x;
            double dy = imgPixel.y - pixel.y;
            double dz = imgPixel.z - pixel.z;
            
            fitnessSum += dx * dx + dy * dy + dz * dz;   
        }
    }

    fitness[i] = fitnessSum;
}

void CudaGLHelper::computeFitness(std::vector<f64>& fitness) {
    if (fitness.size() != resources.size()) {
        std::fprintf(stderr, "CudaGLHelper::computeFitness: fitness.size() != resources.size()\n");
        std::abort();
    }

    std::vector<cudaArray_t> textureArrays(resources.size());
    std::vector<cudaTextureObject_t> textureObjects(resources.size());

    CUDA_CHECK(cudaGraphicsMapResources(resources.size(), resources.data()));
    defer { cudaGraphicsUnmapResources(resources.size(), resources.data()); };

    cudaResourceDesc resDesc;
    memset(&resDesc, 0, sizeof(resDesc));
    resDesc.resType = cudaResourceTypeArray;

    cudaTextureDesc texDesc;
    memset(&texDesc, 0, sizeof(texDesc));
    texDesc.addressMode[0] = cudaAddressModeClamp;
    texDesc.addressMode[1] = cudaAddressModeClamp;
    texDesc.filterMode = cudaFilterModePoint;
    texDesc.readMode = cudaReadModeElementType;
    texDesc.normalizedCoords = 0;

    for (i32 i = 0; i < resources.size(); ++i) {
        // Get mapped arrays
        CUDA_CHECK(cudaGraphicsSubResourceGetMappedArray(&textureArrays[i], resources[i], 0, 0));
        
        // Update texture object
        resDesc.res.array.array = textureArrays[i];
        CUDA_CHECK(cudaCreateTextureObject(&textureObjects[i], &resDesc, &texDesc, nullptr));
    }

    defer {
        for (i32 i = 0; i < resources.size(); ++i)
            cudaDestroyTextureObject(textureObjects[i]);
    };

    cudaTextureObject_t* d_textureObjects;
    CUDA_CHECK(cudaMalloc(&d_textureObjects, textureObjects.size() * sizeof(cudaTextureObject_t)));
    defer { cudaFree(d_textureObjects); };
    CUDA_CHECK(cudaMemcpy(d_textureObjects, textureObjects.data(), textureObjects.size() * sizeof(cudaTextureObject_t), cudaMemcpyHostToDevice));

    // Launch kernel
    i32 BLOCKS = 1;
    i32 THREADS = resources.size();
    computeFitnessKernel<<<BLOCKS, THREADS>>>(
            imageInDevice,
            d_textureObjects,
            fitnessInDevice, width, height);
    CUDA_CHECK(cudaMemcpy(fitness.data(), fitnessInDevice, fitness.size() * sizeof(f64), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaDeviceSynchronize());
}

void CudaGLHelper::unregisterTextures() {
    // TODO
}

GA_NAMESPACE_END
