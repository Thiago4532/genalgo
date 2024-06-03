#ifndef GENALGO_SHADER_HPP
#define GENALGO_SHADER_HPP
 
#include <string>
#include "base.hpp"
#include "tsl/cstring_ref.hpp"

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
    Shader(tsl::cstring_ref vertexSource, tsl::cstring_ref fragmentSource); 
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    static Shader fromSource(tsl::cstring_ref vertexPath, tsl::cstring_ref fragmentPath);
    static Shader fromResource(tsl::cstring_ref vertexPath, tsl::cstring_ref fragmentPath);

    void use();

    ShaderAttr getAttr(tsl::cstring_ref name);

    ShaderAttr operator[](tsl::cstring_ref name) {
        return getAttr(name);
    }

    void setInt(ShaderAttr attr, int value);
    void setFloat(ShaderAttr attr, float value);
    void setVec2i(ShaderAttr attr, int x, int y);
    void setVec3i(ShaderAttr attr, int x, int y, int z);
    void setVec2f(ShaderAttr attr, float x, float y);
    void setVec3f(ShaderAttr attr, float x, float y, float z);

    void setInt(tsl::cstring_ref name, int value) {
        setInt(getAttr(name), value);
    }

    void setFloat(tsl::cstring_ref name, float value) {
        setFloat(getAttr(name), value);
    }

    void setVec2i(tsl::cstring_ref name, int x, int y) {
        setVec2i(getAttr(name), x, y);
    }

    void setVec3i(tsl::cstring_ref name, int x, int y, int z) {
        setVec3i(getAttr(name), x, y, z);
    }

    void setVec2f(tsl::cstring_ref name, float x, float y) {
        setVec2f(getAttr(name), x, y);
    }

    void setVec3f(tsl::cstring_ref name, float x, float y, float z) {
        setVec3f(getAttr(name), x, y, z);
    }
private:
    u32 shaderId;
};

GA_NAMESPACE_END

#endif
