//
// Created by clx on 25-4-7.
//

#ifndef TOY_RENDERER_UPDATE_SHADEROPENGL_H
#define TOY_RENDERER_UPDATE_SHADEROPENGL_H

#include "shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class shaderOpenGL : public Shader {
public:
    shaderOpenGL (std::string vertexPath, std::string fragmentPath, std::string geometryPath = "") : Shader()
    {
        mVertexPath = vertexPath;
        mFragmentPath = fragmentPath;
        mGeometryPath = geometryPath;
        mBackendType = OPENGL;
        mProgram = 0;
    }
    void cleanup() override {
        if(mBackendType != OPENGL) return;
        if(!mProgram) glDeleteProgram(mProgram);
        mProgram = 0;
    }
    void init() override;
    void use(){
        if(mBackendType != OPENGL) return;
        glUseProgram(mProgram);
    }
    GLuint compileShaderProgram(const std::string &path, GLenum type);
    void setMat4(const std::string &name, const glm::mat4 &mat) override{
        if (mBackendType != SHADER_BACKEND_TYPE::OPENGL) return;
        glUniformMatrix4fv(glGetUniformLocation(mProgram, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

private:
    unsigned int mProgram;

};


#endif //TOY_RENDERER_UPDATE_SHADEROPENGL_H
