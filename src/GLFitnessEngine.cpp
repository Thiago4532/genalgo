#include "GLFitnessEngine.hpp"

#include <SFML/Window.hpp>
#include <iostream>
#include <thread>
#include "CudaGLHelper.hpp"
#include "DrawableTexture.hpp"
#include "GLStateManager.hpp"
#include "PoorProfiler.hpp"
#include "Shader.hpp"
#include "glad/glad.h"
#include "globalConfig.hpp"

GA_NAMESPACE_BEGIN

class GLFitnessEngine::EngineImpl {
public:
    EngineImpl();
    ~EngineImpl() = default;

    void evaluate(std::vector<Individual>& individuals);
    int main(Population& pop);

private:
    sf::Window window;
    std::vector<DrawableTexture> individualsTxt;
    Shader individualsShader;
    u32 individualsVBO, individualsVAO;
    std::vector<i32> individualsBuffer;
    i32 iWidth, iHeight;
    sf::Context context;

    CudaGLHelper cudaGLHelper;
};

static void throwIfGLError()  {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        throw std::runtime_error("OpenGL error: " + std::to_string(err));
}

GLFitnessEngine::EngineImpl::EngineImpl() :
    iWidth(globalCfg.targetImage.getWidth()),
    iHeight(globalCfg.targetImage.getHeight())
{
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction)))
        throw std::runtime_error("Failed to load GLAD from SFML");

    glViewport(0, 0, iWidth, iHeight);

    glStateManager.enableBlend();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    individualsTxt.resize(globalCfg.populationSize);
    for (DrawableTexture& txt : individualsTxt) {
        txt.genTexture();
        txt.bindTexture();
        glTexImage2D(GL_TEXTURE_2D,
                0, GL_RGBA,
                iWidth, iHeight,
                0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        txt.genFramebuffer();
    }

    glGenVertexArrays(1, &individualsVAO);
    glGenBuffers(1, &individualsVBO);

    // Individuals VBO data layout:
    // a.x a.y rgba
    // b.x b.y rgba
    // c.x c.y rgba

    glBindVertexArray(individualsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, individualsVBO);
    glVertexAttribIPointer(0, 2, GL_INT, 3 * sizeof(i32), (void*)0);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, 3 * sizeof(i32), (void*)(2 * sizeof(i32)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    throwIfGLError();

    individualsShader = Shader::fromResource("shaders/individuals.vert", "shaders/individuals.frag");
    individualsShader.setVec2i("iSize", iWidth, iHeight);
    individualsShader.setInt("sampler", 0);

    std::vector<u32> individualsTexIds;
    individualsTexIds.reserve(individualsTxt.size());
    for (DrawableTexture const& txt : individualsTxt)
        individualsTexIds.push_back(txt.texture());

    cudaGLHelper.registerTextures(individualsTxt.size(), individualsTexIds.data());
}

int GLFitnessEngine::EngineImpl::main(Population& pop) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    i32 count = 0;
    i32 currentGen = 0;
    individualsTxt[currentGen].bindReadFramebuffer();

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                std::cout << "Closing window...\n";
            }
            else if (event.type == sf::Event::Resized)
                glViewport(0, 0, event.size.width, event.size.height);
        }

        count++;
        if (count >= 165) {
            count = 0;
            currentGen++;
            if (currentGen >= individualsTxt.size())
                currentGen = 0;
            individualsTxt[currentGen].bindReadFramebuffer();
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBlitFramebuffer(0, 0, iWidth, iHeight, 0, 0, iWidth, iHeight,
                GL_COLOR_BUFFER_BIT, GL_NEAREST);
        window.display();
    }

    return 0;
}

void GLFitnessEngine::EngineImpl::evaluate(std::vector<Individual>& individuals) {
    if (individuals.size() != globalCfg.populationSize)
        throw std::runtime_error("Invalid population size");

    sOpenGL.restart();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    individualsBuffer.clear();
    for (i32 i = 0; i < individuals.size(); ++i) {
        for (Triangle const& t : individuals[i]) {
            individualsBuffer.push_back(t.a.x);
            individualsBuffer.push_back(t.a.y);
            individualsBuffer.push_back(t.color.toRGBA());

            individualsBuffer.push_back(t.b.x);
            individualsBuffer.push_back(t.b.y);
            individualsBuffer.push_back(t.color.toRGBA());

            individualsBuffer.push_back(t.c.x);
            individualsBuffer.push_back(t.c.y);
            individualsBuffer.push_back(t.color.toRGBA());
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, individualsVBO);
    glBufferData(GL_ARRAY_BUFFER, individualsBuffer.size() * sizeof(i32),
            individualsBuffer.data(), GL_STREAM_DRAW);
    throwIfGLError();

    glBindVertexArray(individualsVAO);
    individualsShader.use();
    i32 offset = 0;
    for (i32 i = 0; i < individualsTxt.size(); ++i) {
        individualsTxt[i].bindDrawFramebuffer();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, offset, 3 * individuals[i].size());
        offset += individuals[i].size() * 3;
    }

    // Performance penalty, but helps profiling.
    glFinish();

    sOpenGL.trigger();
    sCuda.restart();

    // Compute fitness now
    std::vector<f64> fitness(globalCfg.populationSize);
    cudaGLHelper.computeFitness(fitness);
    for (i32 i = 0; i < individuals.size(); ++i)
        individuals[i].setFitness(fitness[i]);

    sCuda.trigger();
}

// Wrapper for the actual implementation of the engine

GLFitnessEngine::GLFitnessEngine():
    impl(std::make_unique<EngineImpl>()) {}

GLFitnessEngine::~GLFitnessEngine() = default;

void GLFitnessEngine::evaluate(std::vector<Individual>& individuals) {
    impl->evaluate(individuals);
}

int GLFitnessEngine::main(Population& pop) {
    return impl->main(pop);
}


GA_NAMESPACE_END
