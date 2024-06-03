#ifndef GENALGO_GLSTATE_HPP
#define GENALGO_GLSTATE_HPP

#include "base.hpp"
#include "glad/glad.h"

GA_NAMESPACE_BEGIN

class GLStateManager {
public:
    void useProgram(u32 program) {
        if (shaderProgram != program) {
            shaderProgram = program;
            glUseProgram(program);
        }
    }

    void enableBlend() {
        if (!blendEnabled) {
            blendEnabled = true;
            glEnable(GL_BLEND);
        }
    }

    void disableBlend() {
        if (blendEnabled) {
            blendEnabled = false;
            glDisable(GL_BLEND);
        }
    }

private:
    u32 shaderProgram = 0;
    bool blendEnabled = false;
};

// Note: This is not thread-safe. It is assumed that the GLStateManager is only used in the main thread.
extern GLStateManager glStateManager;

GA_NAMESPACE_END

#endif // GENALGO_GLSTATE_HPP
