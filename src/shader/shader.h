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
    WIRE,
    SOLID,
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
    virtual void setMat4(const std::string &name, const glm::mat4 &mat) = 0;

protected:
    SHADER_BACKEND_TYPE mBackendType;
    std::string mVertexPath;
    std::string mFragmentPath;
    std::string mGeometryPath;
};


#endif //TOY_RENDERER_UPDATE_SHADER_H
