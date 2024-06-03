#include "DrawableTexture.hpp"

#include "GLStateManager.hpp"
#include <stdexcept>

GA_NAMESPACE_BEGIN

DrawableTexture::DrawableTexture():
    textureId(0), fbo(0) { }

void DrawableTexture::genTexture() {
    if (textureId != 0 || fbo != 0)
        throw std::runtime_error("Texture already created");

    glGenTextures(1, &textureId);
    if (textureId == 0)
        throw std::runtime_error("Failed to generate texture");
}

void DrawableTexture::genFramebuffer() {
    if (fbo != 0)
        throw std::runtime_error("Framebuffer already created");

    glGenFramebuffers(1, &fbo);
    if (fbo == 0)
        throw std::runtime_error("Failed to generate framebuffer");

    glBindTexture(GL_TEXTURE_2D, textureId);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

    u32 status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
        throw std::runtime_error("Framebuffer is not complete: " + std::to_string(status));
    }
}

DrawableTexture::~DrawableTexture() {
    if (textureId != 0)
        glDeleteTextures(1, &textureId);

    if (fbo != 0) 
        glDeleteFramebuffers(1, &fbo);
}

void DrawableTexture::bindTexture() {
    if (textureId != 0)
        glBindTexture(GL_TEXTURE_2D, textureId);
}

void DrawableTexture::bindReadFramebuffer() {
    if (fbo != 0)
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
}

void DrawableTexture::bindDrawFramebuffer() {
    if (fbo != 0)
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
}

void DrawableTexture::bindFramebuffer() {
    if (fbo != 0)
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

}

GA_NAMESPACE_END
