//
// Created by clx on 25-4-7.
//

#include "shaderOpenGL.h"
#include <iostream>
#include <fstream>
#include <sstream>

void shaderOpenGL::init()
{
    if(mBackendType != SHADER_BACKEND_TYPE::OPENGL)return;
    GLuint vertexShader = compileShaderProgram(mVertexPath, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShaderProgram(mFragmentPath, GL_FRAGMENT_SHADER);
    GLuint geometryShader = 0;
    if (!mGeometryPath.empty()) {
        geometryShader = compileShaderProgram(mGeometryPath, GL_GEOMETRY_SHADER);
    }

    mProgram = glCreateProgram();
    glAttachShader(mProgram, vertexShader);
    glAttachShader(mProgram, fragmentShader);
    if (geometryShader) { glAttachShader(mProgram, geometryShader); }
    glLinkProgram(mProgram);

    GLint success;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(mProgram, 512, nullptr, infoLog);
        std::cerr << "Error linking shader program: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (geometryShader) { glDeleteShader(geometryShader); }
}

GLuint shaderOpenGL::compileShaderProgram(const std::string &path, GLenum type) {
    std::ifstream file;
    std::stringstream buffer;
    std::string code;
    file.exceptions (std::ifstream::failbit | std::ifstream::badbit);

    try {
        file.open(path);
        buffer << file.rdbuf();
        code = buffer.str();
    } catch (std::ifstream::failure& e) {
        std::cerr << "Error opening shader file: " << path << std::endl << e.what() << std::endl;
    }

    const char* shaderCode = code.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderCode, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Error compiling shader: " << infoLog << std::endl;
    }

    return shader;
}

void Shader::setBool(const std::string &name, bool value) const {
    if (mBackendType != SHADER_BACKEND_TYPE::OPENGL) return;
    glUniform1i(glGetUniformLocation(mProgram, name.c_str()), static_cast<int>(value));
}

void Shader::setInt(const std::string &name, int value) const {
    if (mBackendType != SHADER_BACKEND_TYPE::OPENGL) return;
    glUniform1i(glGetUniformLocation(mProgram, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const {
    if (mBackendType != SHADER_BACKEND_TYPE::OPENGL) return;
    glUniform1f(glGetUniformLocation(mProgram, name.c_str()), value);
}

void Shader::setVec3(const std::string &name, const glm::vec3 &value) const {
    if (mBackendType != SHADER_BACKEND_TYPE::OPENGL) return;
    glUniform3fv(glGetUniformLocation(mProgram, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const {
    if (mBackendType != SHADER_BACKEND_TYPE::OPENGL) return;
    glUniformMatrix4fv(glGetUniformLocation(mProgram, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}