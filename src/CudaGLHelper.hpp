#ifndef GENALGO_CUDAGLHELPER_HPP
#define GENALGO_CUDAGLHELPER_HPP

#include "base.hpp"
#include "Vec.hpp"
#include <vector>

struct cudaGraphicsResource;

GA_NAMESPACE_BEGIN

class CudaGLHelper {
public:
    CudaGLHelper();
    ~CudaGLHelper();
    
    CudaGLHelper(const CudaGLHelper&) = delete;
    CudaGLHelper& operator=(const CudaGLHelper&) = delete;

    void registerTextures(i32 count, u32 textures[]);
    void unregisterTextures();

    void computeFitness(std::vector<f64>& fitness);
private:
    std::vector<cudaGraphicsResource*> resources;
    Vec3d* imageInDevice;
    f64* fitnessInDevice;
    i32 width, height;
};

GA_NAMESPACE_END

#endif // GENALGO_CUDAGLHELPER_HPP
