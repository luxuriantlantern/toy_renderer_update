//
// Created by clx on 25-4-7.
//

#ifndef TOY_RENDERER_UPDATE_SHADER_H
#define TOY_RENDERER_UPDATE_SHADER_H

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

enum SHADER_TYPE
{
    WIREFRAME,
    Blinn_Phong,
    MATERIAL
};

enum SHADER_BACKEND_TYPE
{
    OPENGL,
    VULKAN
};

class Shader {
public:
    virtual void cleanup() = 0;
    virtual void init() = 0;
    SHADER_BACKEND_TYPE getBackendType() const { return mBackendType; }
    virtual void use() = 0;

    virtual void setBool(const std::string &name, bool value) const = 0;
    virtual void setInt(const std::string &name, int value) const = 0;
    virtual void setFloat(const std::string &name, float value) const = 0;
    virtual void setVec3(const std::string &name, const glm::vec3 &value) const = 0;
    virtual void setMat4(const std::string &name, const glm::mat4 &mat) const = 0;

protected:
    SHADER_TYPE mShaderType;
    SHADER_BACKEND_TYPE mBackendType;
    std::string mVertexPath;
    std::string mFragmentPath;
    std::string mGeometryPath;
    unsigned int mProgram;
};


#endif //TOY_RENDERER_UPDATE_SHADER_H
