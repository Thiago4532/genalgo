#ifndef GENALGO_DRAWABLETEXTURE_HPP
#define GENALGO_DRAWABLETEXTURE_HPP

#include "base.hpp"

GA_NAMESPACE_BEGIN

class DrawableTexture {
public:
    DrawableTexture();
    ~DrawableTexture();

    void genTexture();
    void genFramebuffer();

    u32 texture() const { return textureId; }
    u32 framebuffer() const { return fbo; }

    void bindTexture();
    void bindReadFramebuffer();
    void bindDrawFramebuffer();
    void bindFramebuffer();

private:
    u32 textureId;
    u32 fbo;
};

GA_NAMESPACE_END

#endif // GENALGO_DRAWABLETEXTURE_HPP
