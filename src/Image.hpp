#ifndef GENALGO_IMAGE_HPP
#define GENALGO_IMAGE_HPP

#include "base.hpp"
#include <string>

GA_NAMESPACE_BEGIN

// Image RGBA
class Image {
public:
    Image() noexcept;
    
    // TODO: Implement a copy/move constructor
    Image(const Image& other) = delete;
    Image(Image&& other) = delete;
    Image& operator=(const Image& other) = delete;
    Image& operator=(Image&& other) = delete;

    Image(u32 width, u32 height);
    bool load(std::string const& filename);

    void computeWeights();

    i32 getWidth() const noexcept { return width; }
    i32 getHeight() const noexcept { return height; }

    u8* getData() noexcept { return data; }
    f64* getWeights() noexcept { return weights; }

    ~Image();
private:
    u8* data;
    f64* weights;

    i32 width;
    i32 height;
};

GA_NAMESPACE_END

#endif // GENALGO_IMAGE_HPP
