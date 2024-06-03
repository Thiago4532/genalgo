#version 330 core
layout (location = 0) in vec2i mPos;
layout (location = 1) in uuint mColor;

out vec2 texCoord;

uniform vec2 resolution;
uniform vec2i iSize;

void main() {
    gl_Position = vec4(mPos, 0.0f, 1.0f);
    vec2 normalized = (mPos + 1.0f) / 2.0f;
    texCoord = vec2(normalized.x, 1.0f - normalized.y) * resolution / imSize;
}
