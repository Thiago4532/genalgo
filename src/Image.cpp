#include "Image.hpp"

#include <SFML/Graphics.hpp>
#include <cstring>

GA_NAMESPACE_BEGIN

Image::Image() noexcept: data(nullptr), width(0), height(0) {}

Image::Image(unsigned int width, unsigned int height): width(width), height(height) {
    data = new unsigned char[width * height * 4];
}

bool Image::load(std::string const& filename) {
    sf::Image sfImage;
    if (!sfImage.loadFromFile(filename))
        return false;

    if (data)
        delete[] data;

    width = sfImage.getSize().x;
    height = sfImage.getSize().y;
    data = new unsigned char[width * height * 4];

    std::memcpy(data, sfImage.getPixelsPtr(), width * height * 4);
    return true;
}

Image::~Image() {
    delete[] data;
}

GA_NAMESPACE_END
