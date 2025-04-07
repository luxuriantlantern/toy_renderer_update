//
// Created by clx on 25-4-7.
//

#include "render_OpenGL.h"

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

    }


    mModelResources[model] = resources;
}