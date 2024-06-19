#ifndef GENALGO_SHADER_HPP
#define GENALGO_SHADER_HPP
 
#include <string>
#include "base.hpp"

GA_NAMESPACE_BEGIN

// TODO: Implement a type-safe wrapper for Shader attributes
class ShaderAttr {
public:
    explicit ShaderAttr(int location)
        : location(location) {}
    ShaderAttr()
        : location(-1) {}

    ~ShaderAttr() {}

    int getLocation() const { return location; }

private:
    int location;
};
  
class Shader {
public:  
    Shader(): shaderId(0) { }
    Shader(std::string const& vertexSource, std::string const& fragmentSource); 
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    static Shader fromSource(std::string const& vertexPath, std::string const& fragmentPath);
    static Shader fromResource(std::string const& vertexPath, std::string const& fragmentPath);

    void use();

    ShaderAttr getAttr(std::string const& name);
    ShaderAttr getAttr(const char* name);

    ShaderAttr operator[](std::string const& name) {
        return getAttr(name);
    }

    void setInt(ShaderAttr attr, int value);
    void setFloat(ShaderAttr attr, float value);
    void setVec2i(ShaderAttr attr, int x, int y);
    void setVec3i(ShaderAttr attr, int x, int y, int z);
    void setVec2f(ShaderAttr attr, float x, float y);
    void setVec3f(ShaderAttr attr, float x, float y, float z);

    void setInt(std::string const& name, int value) {
        setInt(getAttr(name), value);
    }

    void setFloat(std::string const& name, float value) {
        setFloat(getAttr(name), value);
    }

    void setVec2i(std::string const& name, int x, int y) {
        setVec2i(getAttr(name), x, y);
    }

    void setVec3i(std::string const& name, int x, int y, int z) {
        setVec3i(getAttr(name), x, y, z);
    }

    void setVec2f(std::string const& name, float x, float y) {
        setVec2f(getAttr(name), x, y);
    }

    void setVec3f(std::string const& name, float x, float y, float z) {
        setVec3f(getAttr(name), x, y, z);
    }
private:
    u32 shaderId;
};

GA_NAMESPACE_END

#endif
