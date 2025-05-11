#include "Image.hpp"

#include <SFML/Graphics.hpp>
#include <cstring>
#include <iostream>
#include <memory>

GA_NAMESPACE_BEGIN

Image::Image() noexcept: data(nullptr), width(0), height(0) {}

Image::Image(unsigned int width, unsigned int height): width(width), height(height) {
    data = new unsigned char[width * height * 4];
    weights = new f64[width * height];
}

// Neighbor-difference-squared
void Image::computeWeights() {
    const i32 RADIUS = 5;

    auto sums = std::make_unique<f64[]>(width * height);
    for (i32 y = 0; y < height; ++y) {
        for (i32 x = 0; x < width; ++x) {
            i64 total = 0;
            i32 numNeighbours = 0;
            for (i32 dy = -RADIUS; dy <= RADIUS; ++dy) {
                for (i32 dx = -RADIUS; dx <= RADIUS; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    auto [xN, yN] = std::make_pair(x + dx, y + dy);
                    if (xN < 0 || xN >= width || yN < 0 || yN >= height) continue;

                    ++numNeighbours;
                    i32 idxN = (yN * width + xN) * 4;
                    i32 idx = (y * width + x) * 4;
                    for (i32 i = 0; i < 4; ++i) {
                        i64 diff = data[idxN + i] - data[idx + i];
                        total += diff * diff;
                    }
                }
            }

            if (numNeighbours > 0) {
                f64 sum = 1.0 * total / (255 * 255 * numNeighbours);
                sums[y * width + x] = sum;
            } else {
                sums[y * width + x] = 0.0;
            }
        }
    }

    // Normalize the weights
    for (i32 y = 0; y < height; ++y) {
        for (i32 x = 0; x < width; ++x) {
            i32 idx = (y * width + x);
            weights[idx] = 0;
            // weights[idx] = sums[idx];
        }
    }
}

// Kernel-based edge detection
// void Image::computeWeights() {
//     const i32 KERNEL_SIZE = 3;

//     // Sobel kernels for edge detection
//     const i32 Gx[KERNEL_SIZE][KERNEL_SIZE] = {
//         {-1, 0, 1},
//         {-2, 0, 2},
//         {-1, 0, 1}
//     };

//     const i32 Gy[KERNEL_SIZE][KERNEL_SIZE] = {
//         {-1, -2, -1},
//         { 0,  0,  0},
//         { 1,  2,  1}
//     };

//     auto gradients = std::make_unique<f64[]>(width * height);

//     for (i32 y = 0; y < height; ++y) {
//         for (i32 x = 0; x < width; ++x) {
//             f64 sumX = 0.0;
//             f64 sumY = 0.0;

//             for (i32 ky = -1; ky <= 1; ++ky) {
//                 for (i32 kx = -1; kx <= 1; ++kx) {
//                     auto [xN, yN] = std::make_pair(x + kx, y + ky);
//                     if (xN < 0 || xN >= width || yN < 0 || yN >= height) continue;

//                     i32 idxN = (yN * width + xN) * 4;
//                     i32 gray = (data[idxN] + data[idxN + 1] + data[idxN + 2]) / 3;

//                     sumX += gray * Gx[ky + 1][kx + 1];
//                     sumY += gray * Gy[ky + 1][kx + 1];
//                 }
//             }

//             gradients[y * width + x] = std::sqrt(sumX * sumX + sumY * sumY);
//         }
//     }

//     f64 maxGradient = 0.0;
//     for (i32 i = 0; i < width * height; ++i) {
//         if (gradients[i] > maxGradient) {
//             maxGradient = gradients[i];
//         }
//     }

//     for (i32 i = 0; i < width * height; ++i) {
//         weights[i] = gradients[i] / maxGradient;
//     }
// }

bool Image::load(std::string const& filename) {
    sf::Image sfImage;
    if (!sfImage.loadFromFile(filename))
        return false;

    if (data)
        delete[] data;

    width = sfImage.getSize().x;
    height = sfImage.getSize().y;
    data = new unsigned char[width * height * 4];
    weights = new f64[width * height];

    std::memcpy(data, sfImage.getPixelsPtr(), width * height * 4);

    computeWeights();
    return true;
}

Image::~Image() {
    delete[] data;
    delete[] weights;
}

GA_NAMESPACE_END
