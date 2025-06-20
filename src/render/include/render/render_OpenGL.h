//
// Created by clx on 25-4-7.
//

#ifndef TOY_RENDERER_UPDATE_RENDER_OPENGL_H
#define TOY_RENDERER_UPDATE_RENDER_OPENGL_H

#include "render.h"
#include "shader/shaderOpenGL.h"


class Render_OpenGL : public Render {
public:
    Render_OpenGL() = default;
    ~Render_OpenGL() override;
    void setup(const std::shared_ptr<Scene>& scene) override;
    void addModel(const std::shared_ptr<Object>& model) override;
    void removeModel(const std::shared_ptr<Object>& model) override;
    void init() override;
    void render(
        const std::shared_ptr<Scene>& scene,
        const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix
    ) override;
    SHADER_BACKEND_TYPE getType() override { return SHADER_BACKEND_TYPE::OPENGL; }
    void setShaderType(SHADER_TYPE type) override {
        mCurrentShader = { type, mShaders[type] };
    }
    SHADER_TYPE getShaderType() const override {
        return mCurrentShader.first;
    }
    std::optional<std::reference_wrapper<easyVulkan::renderPassWithFramebuffers>> RenderPassAndFramebuffers() override {
        throw std::logic_error("OpenGL backend does not support RenderPassAndFramebuffers");
    }
    void resetRPWF() override {
        throw std::logic_error("OpenGL backend does not support resetRPWF");
    }

private:
    struct OpenGLModelResources {
        std::vector<GLuint> VAOs;
        std::vector<GLuint> VBOs;
        std::vector<GLuint> textures;
        std::vector<size_t> vertexCounts;
    };
    void cleanup() override;
    std::unordered_map<std::shared_ptr<Object>, OpenGLModelResources> mModelResources;
    void loadTexture(const std::string& path, GLuint& textureID);
};


#endif //TOY_RENDERER_UPDATE_RENDER_OPENGL_H
