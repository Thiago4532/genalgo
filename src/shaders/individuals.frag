#version 430 core

flat in vec4 normColor;
out vec4 FragColor;

void main() {
    FragColor = normColor;
}
