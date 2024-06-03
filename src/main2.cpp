#include <iostream>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "glad/glad.h"
#include "base.hpp"
#include "Shader.hpp"

GA_NAMESPACE_BEGIN

int main() {
    sf::Image image;
    if (!image.loadFromFile("maomao.jpeg")) {
        std::cerr << "Failed to load maomao image!\n";
        return 1;
    }
    auto [imWidth, imHeight] = image.getSize();
    auto width = imWidth;
    auto height = imHeight;

    sf::Window window(sf::VideoMode(width, height), "genalgo", sf::Style::Default);
    window.setVerticalSyncEnabled(true);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) {
        std::cerr << "Failed to load GLAD\n";
        return 1;
    }

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imWidth, imHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());

    float imBox[] = {
        -1, -1,
        1, -1,
        1, 1,
        -1, 1
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(imBox), imBox, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    Shader shader = Shader::fromSource("shaders/shader.vert", "shaders/shader.frag");

    shader.setVec2f("resolution", width, height);
    shader.setVec2f("imSize", imWidth, imHeight);

    bool running = true;
    while (running) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                running = false;
            } else if (event.type == sf::Event::Resized) {
                width = event.size.width;
                height = event.size.height;
                glViewport(0, 0, width, height);
                shader.setVec2f("resolution", width, height);
            }
        }

        shader.use();
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        window.display();
    }

    return 0;
}

GA_NAMESPACE_END

// Entry point
int main() {
    return genalgo::main();
}
