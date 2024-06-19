#include "Shader.hpp"

#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include "defer.hpp"
#include "GLStateManager.hpp"
#include "resource.hpp"

using namespace std;

GA_NAMESPACE_BEGIN

// TODO: Reimplement this, maybe using memory mapped files
Shader Shader::fromSource(std::string const& vertexPath, std::string const& fragmentPath) {
    string vertexCode;
    string fragmentCode;
    ifstream vShaderFile;
    ifstream fShaderFile;

    // TODO: Better error handling, the exception is not clear
    vShaderFile.exceptions (ifstream::failbit | ifstream::badbit);
    fShaderFile.exceptions (ifstream::failbit | ifstream::badbit);
    vShaderFile.open(vertexPath.data());
    fShaderFile.open(fragmentPath.data());
    stringstream vShaderStream, fShaderStream;

    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();		

    vShaderFile.close();
    fShaderFile.close();

    vertexCode   = vShaderStream.str();
    fragmentCode = fShaderStream.str();		

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    return Shader(vShaderCode, fShaderCode);
}

Shader Shader::fromResource(std::string const& vertexPath, std::string const& fragmentPath) {
    return fromSource(getResourcePath(vertexPath.data()), getResourcePath(fragmentPath.data()));
}

Shader::Shader(std::string const& vShaderCode_, std::string const& fShaderCode_) {
    const char* vShaderCode = vShaderCode_.data();
    const char* fShaderCode = fShaderCode_.data();

    u32 vertex, fragment;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        throw runtime_error("ERROR::SHADER::VERTEX::COMPILATION_FAILED: "s + infoLog);
    };
    defer { glDeleteShader(vertex); };

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        throw runtime_error("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED: "s + infoLog);
    };
    defer { glDeleteShader(fragment); };

    shaderId = glCreateProgram();
    glAttachShader(shaderId, vertex);
    glAttachShader(shaderId, fragment);
    glLinkProgram(shaderId);

    glGetProgramiv(shaderId, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(shaderId, 512, NULL, infoLog);
        throw runtime_error("ERROR::SHADER::PROGRAM::LINKING_FAILED: "s + infoLog);
    }
}

Shader::~Shader() {
    if (!shaderId)
        return;

    glDeleteProgram(shaderId);
    glStateManager.useProgram(0);
}

Shader::Shader(Shader&& other) noexcept {
    shaderId = other.shaderId;
    other.shaderId = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this == &other)
        return *this;

    if (shaderId)
        glDeleteProgram(shaderId);

    shaderId = other.shaderId;
    other.shaderId = 0;
    return *this;
}

void Shader::use() {
    glStateManager.useProgram(shaderId);
}

ShaderAttr Shader::getAttr(const char* name) {
    return ShaderAttr(glGetUniformLocation(shaderId, name));
}

ShaderAttr Shader::getAttr(std::string const& name) {
    return getAttr(name.data());
}

void Shader::setFloat(ShaderAttr attr, float value) {
    use();
    glUniform1f(attr.getLocation(), value);
}

void Shader::setInt(ShaderAttr attr, int value) {
    use();
    glUniform1i(attr.getLocation(), value);
}

void Shader::setVec2i(ShaderAttr attr, int x, int y) {
    use();
    glUniform2i(attr.getLocation(), x, y);
}

void Shader::setVec3i(ShaderAttr attr, int x, int y, int z) {
    use();
    glUniform3i(attr.getLocation(), x, y, z);
}

void Shader::setVec2f(ShaderAttr attr, float x, float y) {
    use();
    glUniform2f(attr.getLocation(), x, y);
}

void Shader::setVec3f(ShaderAttr attr, float x, float y, float z) {
    use();
    glUniform3f(attr.getLocation(), x, y, z);
}

GA_NAMESPACE_END
