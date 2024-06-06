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
    void update();

private:
    SFMLRenderer& self;
    std::thread renderThread;
    std::atomic<bool> flagExit;

    // Render thread only
    sf::RenderTexture renderTexture;
    i32 index = -1;
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

    sf::RenderWindow window(sf::VideoMode(width * scale, height * scale), "Genetic Algorithm - Best Individual", sf::Style::Default & ~sf::Style::Resize);
    window.setVerticalSyncEnabled(true);
    window.setActive(true);

    renderTexture.create(width * scale, height * scale);

    sf::Image targetImage;
    targetImage.create(width, height, globalCfg.targetImage.getData());
    sf::Texture targetTexture;
    targetTexture.loadFromImage(targetImage);

    sf::Sprite sprite(renderTexture.getTexture());
    // sprite.setScale(
    //     static_cast<float>(scale),
    //     static_cast<float>(scale)
    // );
    sf::Sprite targetSprite(targetTexture);
    targetSprite.setPosition(width * scale, 0);
    targetSprite.setScale(
        static_cast<float>(scale),
        static_cast<float>(scale)
    );

    bool showOriginal = false;

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
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::S) {
                    showOriginal = !showOriginal;
                    window.setSize(sf::Vector2u(width * scale * (showOriginal ? 2 : 1), height * scale));
                } else if (event.key.code == sf::Keyboard::R) {
                    index = -1;
                    window.setTitle("Genetic Algorithm - Best Individual");
                    update();
                } else if (event.key.code == sf::Keyboard::N) {
                    index++;
                    if (index >= globalCfg.populationSize)
                        index = 0;
                    window.setTitle("Genetic Algorithm - Individual #" + std::to_string(index));
                    update();
                } else if (event.key.code == sf::Keyboard::P) {
                    index--;
                    if (index < 0)
                        index = globalCfg.populationSize - 1;
                    window.setTitle("Genetic Algorithm - Individual #" + std::to_string(index));
                    update();
                }
            }
        }

        processRenderRequest();

        window.clear(sf::Color::Black);
        window.draw(sprite);
        window.draw(targetSprite);
        window.display();
    }
    self.renderExited = true;
}

void SFMLRenderer::RendererImpl::processRenderRequest() {
    if (!self.renderRequested)
        return;

    update();
    self.renderRequested = false;
}

void SFMLRenderer::RendererImpl::update() {
    auto& individual = index == -1 ? self.bestIndividual : self.population.getIndividuals()[index];
    i32 width = globalCfg.targetImage.getWidth();
    i32 height = globalCfg.targetImage.getHeight();

    sf::VertexArray vA(sf::Triangles, individual.size() * 3);

    float scale = globalCfg.renderScale;
    for (Triangle const& t : individual) {
        sf::Color color(t.color.r, t.color.g, t.color.b, t.color.a);
        vA.append(sf::Vertex(scale * sf::Vector2f(t.a.x, height - t.a.y), color));
        vA.append(sf::Vertex(scale * sf::Vector2f(t.b.x, height - t.b.y), color));
        vA.append(sf::Vertex(scale * sf::Vector2f(t.c.x, height - t.c.y), color));
    }

    renderTexture.clear();
    renderTexture.draw(vA);
}

SFMLRenderer::SFMLRenderer()
    : renderRequested(false),
      renderExited(false),
      impl(std::make_unique<RendererImpl>(*this))
{ }

SFMLRenderer::~SFMLRenderer() = default;

GA_NAMESPACE_END
