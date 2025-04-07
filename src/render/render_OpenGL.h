//
// Created by clx on 25-4-7.
//

#ifndef TOY_RENDERER_UPDATE_RENDER_OPENGL_H
#define TOY_RENDERER_UPDATE_RENDER_OPENGL_H

#include "render.h"

struct OpenGLModelResources {
    std::vector<GLuint> VAOs;
    std::vector<GLuint> VBOs;
    std::vector<GLuint> textures;
    std::vector<size_t> vertexCounts;
};

class Render_OpenGL : public Render {
public:
    Render_OpenGL() = default;
    ~Render_OpenGL() override;
    void setup(const std::shared_ptr<Scene>& scene) override;
    void addModel(const std::shared_ptr<Object>& model) override;
    void removeModel(const std::shared_ptr<Object>& model) override;


private:
    void cleanup() override;
    std::unordered_map<std::shared_ptr<Object>, OpenGLModelResources> mModelResources;
};


#endif //TOY_RENDERER_UPDATE_RENDER_OPENGL_H
