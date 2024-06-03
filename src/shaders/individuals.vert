#version 430 core
layout (location = 0) in ivec2 mPos;
layout (location = 1) in uint mColor;

flat out vec4 normColor;

uniform ivec2 iSize;

void main() {
    vec2 pos = vec2(mPos) / vec2(iSize);
    pos.y = 1.0f - pos.y;
    pos = pos * 2.0f - 1.0f;

    uint r = (mColor >> 24) & 255u;
    uint g = (mColor >> 16) & 255u;
    uint b = (mColor >> 8) & 255u;
    uint a = mColor & 255u;

    normColor = vec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    gl_Position = vec4(pos, 0.0f, 1.0f);
}

