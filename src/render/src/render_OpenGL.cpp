//
// Created by clx on 25-4-7.
//

#include "render/render_OpenGL.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

void Render_OpenGL::init()
{
    mShaders[SHADER_TYPE::MATERIAL] = std::make_shared<shaderOpenGL>(
        "./assets/shaders/material.vert",
        "./assets/shaders/material.frag"
        );
    mShaders[SHADER_TYPE::WIREFRAME] = std::make_shared<shaderOpenGL>(
        "./assets/shaders/wireframe.vert",
        "./assets/shaders/wireframe.frag"
        );
    mShaders[SHADER_TYPE::Blinn_Phong] = std::make_shared<shaderOpenGL>(
        "./assets/shaders/Blinn-Phong.vert",
        "./assets/shaders/Blinn-Phong.frag"
        );
    for(auto & shader : mShaders)
    {
        shader.second->init();
    }
    mCurrentShader = { SHADER_TYPE::MATERIAL, mShaders[SHADER_TYPE::MATERIAL] };
    glEnable(GL_DEPTH_TEST);
}

void Render_OpenGL::render(const std::shared_ptr<Scene>& scene, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    glClearColor(0.00f, 0.00f, 0.00f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, mCurrentShader.first == SHADER_TYPE::WIREFRAME ? GL_LINE : GL_FILL);
    glLineWidth(1.0f);

    auto shader = mCurrentShader.second;
    shader->use();

    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);

    auto models = scene->getModels();
    for (const auto& model : models) {
        shader->setMat4("model", model->getModelMatrix());
        const OpenGLModelResources& resources = mModelResources.at(model);
        size_t shapeCount = model->getShapeCount();
        for (size_t i = 0; i < shapeCount; ++i) {
            glBindVertexArray(resources.VAOs[i]);
            shader->setBool("hasTexture", resources.textures[i] != 0);
            if (resources.textures[i]) {
                // Use GL_TETURE0 all the time
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, resources.textures[i]);
                shader->setInt("textureDiffuse", 0);
            }

            glDrawArrays(GL_TRIANGLES, 0, resources.vertexCounts[i]);
            glBindVertexArray(0);
        }
    }
}

Render_OpenGL::~Render_OpenGL() {
    Render_OpenGL::cleanup();
    for (auto& shader : mShaders) {
        shader.second->cleanup();
    }
}

void Render_OpenGL::cleanup() {
    for (auto& model : mModelResources) {
        auto& resources = model.second;
        for (auto& vao : resources.VAOs) {
            glDeleteVertexArrays(1, &vao);
        }
        for (auto& vbo : resources.VBOs) {
            glDeleteBuffers(1, &vbo);
        }
        for (auto& texture : resources.textures) {
            glDeleteTextures(1, &texture);
        }
    }
    mModelResources.clear();
}

void Render_OpenGL::setup(const std::shared_ptr<Scene> &scene) {
    cleanup();
    for (const auto& model : scene->getModels()) {
        addModel(model);
    }
}

void Render_OpenGL::addModel(const std::shared_ptr<Object> &model) {
    size_t shapeCount = model->getShapeCount();
    OpenGLModelResources resources;
    resources.VAOs.resize(shapeCount);
    resources.VBOs.resize(shapeCount);
    resources.textures.resize(shapeCount);
    resources.vertexCounts.resize(shapeCount);

    glGenVertexArrays(shapeCount, resources.VAOs.data());
    glGenBuffers(shapeCount, resources.VBOs.data());

    for(size_t i = 0; i < shapeCount; ++i)
    {
        const std::vector<glm::vec3>& vertices = model->getVertices(i);
        const std::vector<glm::vec3>& normals = model->getNormals(i);
        const std::vector<glm::vec2>& texCoords = model->getTexCoords(i);
        const std::string& texturePath = model->getTexturePath(i);

        std::vector<float> buffer;
        for(size_t j = 0; j < vertices.size(); ++j)
        {
            buffer.push_back(vertices[j].x);
            buffer.push_back(vertices[j].y);
            buffer.push_back(vertices[j].z);

            if(!normals.empty())
            {
                buffer.push_back(normals[j].x);
                buffer.push_back(normals[j].y);
                buffer.push_back(normals[j].z);
            }

            if(!texCoords.empty())
            {
                buffer.push_back(texCoords[j].x);
                buffer.push_back(texCoords[j].y);
            }
        }

        glBindVertexArray(resources.VAOs[i]);

        glBindBuffer(GL_ARRAY_BUFFER, resources.VBOs[i]);
        glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(float), buffer.data(), GL_STATIC_DRAW);

        size_t stride = 3;
        if (!normals.empty()) stride += 3;
        if (!texCoords.empty()) stride += 2;

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)nullptr);
        glEnableVertexAttribArray(0);
        size_t offset = 3 * sizeof(float);
        resources.vertexCounts[i] = vertices.size();

        if (!normals.empty()) {
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)offset);
            glEnableVertexAttribArray(1);
            offset += 3 * sizeof(float);
        }

        if (!texCoords.empty()) {
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)offset);
            glEnableVertexAttribArray(2);
        }

        // Load texture
        if (!texturePath.empty()) {
            loadTexture(texturePath, resources.textures[i]);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    mModelResources[model] = resources;
}

void Render_OpenGL::removeModel(const std::shared_ptr<Object> &model) {
    auto it = mModelResources.find(model);
    if (it != mModelResources.end()) {
        auto& resources = it->second;
        for (auto& vao : resources.VAOs) {
            glDeleteVertexArrays(1, &vao);
        }
        for (auto& vbo : resources.VBOs) {
            glDeleteBuffers(1, &vbo);
        }
        for (auto& texture : resources.textures) {
            glDeleteTextures(1, &texture);
        }
        mModelResources.erase(it);
    }
}

void Render_OpenGL::loadTexture(const std::string& path, GLuint& textureID) {

    stbi_set_flip_vertically_on_load(true);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}