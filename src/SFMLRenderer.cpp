#include "SFMLRenderer.hpp"
#include "globalConfig.hpp"

#include <iostream>
#include <thread>
#include <SFML/Graphics.hpp>

GA_NAMESPACE_BEGIN

class SFMLRenderer::RendererImpl {
public:
    RendererImpl(SFMLRenderer& self);
    ~RendererImpl();

    void renderLoop();
    void processRenderRequest();

private:
    SFMLRenderer& self;
    std::thread renderThread;
    std::atomic<bool> flagExit;

    // Render thread only
    sf::RenderTexture renderTexture;
};

SFMLRenderer::RendererImpl::RendererImpl(SFMLRenderer& self)
    : self(self),
      flagExit(false)
{
    renderThread = std::thread(&RendererImpl::renderLoop, this);
}

SFMLRenderer::RendererImpl::~RendererImpl() {
    flagExit.store(true, std::memory_order_relaxed);
    renderThread.join();
}

void SFMLRenderer::RendererImpl::renderLoop() {
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();

    i32 scale = globalCfg.renderScale;

    sf::RenderWindow window(sf::VideoMode(width * scale, height * scale), "Genetic Algorithm", sf::Style::Default);
    window.setVerticalSyncEnabled(true);
    window.setActive(true);

    renderTexture.create(width * scale, height * scale);

    sf::Sprite sprite(renderTexture.getTexture());
    // sprite.setScale(
    //     static_cast<float>(scale),
    //     static_cast<float>(scale)
    // );

    while (window.isOpen()) {
        if (flagExit.load(std::memory_order_relaxed)) {
            window.close();
            break;
        }

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::Resized) {
                // sprite.setScale(
                //     static_cast<float>(event.size.width) / width,
                //     static_cast<float>(event.size.height) / height
                // );
            }
        }

        processRenderRequest();

        window.clear(sf::Color::Black);
        window.draw(sprite);
        window.display();
    }
    self.renderExited = true;
}

void SFMLRenderer::RendererImpl::processRenderRequest() {
    if (!self.renderRequested)
        return;

    auto& bestIndividual = self.bestIndividual;
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();

    sf::VertexArray vA(sf::Triangles, bestIndividual.size() * 3);

    float scale = globalCfg.renderScale;
    for (Triangle const& t : bestIndividual) {
        sf::Color color(t.color.r, t.color.g, t.color.b, t.color.a);
        vA.append(sf::Vertex(scale * sf::Vector2f(t.a.x, t.a.y), color));
        vA.append(sf::Vertex(scale * sf::Vector2f(t.b.x, t.b.y), color));
        vA.append(sf::Vertex(scale * sf::Vector2f(t.c.x, t.c.y), color));
    }

    renderTexture.clear();
    renderTexture.draw(vA);
    self.renderRequested = false;
}

SFMLRenderer::SFMLRenderer()
    : renderRequested(false),
      renderExited(false),
      impl(std::make_unique<RendererImpl>(*this))
{ }

SFMLRenderer::~SFMLRenderer() = default;

GA_NAMESPACE_END
