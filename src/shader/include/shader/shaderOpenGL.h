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
    void use() override{
        if(mBackendType != OPENGL) return;
        glUseProgram(mProgram);
    }
    GLuint compileShaderProgram(const std::string &path, GLenum type);
    void setBool(const std::string &name, bool value) const override;
    void setInt(const std::string &name, int value) const override;
    void setFloat(const std::string &name, float value) const override;
    void setVec3(const std::string &name, const glm::vec3 &value) const override;
    void setMat4(const std::string &name, const glm::mat4 &mat) const override;

    semaphore& getSemaphoreImageIsAvailable() override {
        throw std::logic_error("OpenGL backend does not support getSemaphoreImageIsAvailable");
    }
    uniformBuffer& getUniformBuffer() override {
        throw std::logic_error("OpenGL backend does not support getUniformBuffer");
    }
//    renderPass& getRenderPass() override {
//        throw std::logic_error("OpenGL backend does not support getRenderPass");
//    }
//    std::vector<framebuffer>& getFramebuffer() override {
//        throw std::logic_error("OpenGL backend does not support getFramebuffer");
//    }
    std::optional<commandBuffer> & getCommandBuffer() override {
        throw std::logic_error("OpenGL backend does not support getCommandBuffer");
    }
    fence& getFence() override {
        throw std::logic_error("OpenGL backend does not support getFence");
    }
    semaphore& getSemaphoreRenderingIsOver() override {
        throw std::logic_error("OpenGL backend does not support getSemaphoreRenderingIsOver");
    }
    VkClearValue* getClearValue() override {
        throw std::logic_error("OpenGL backend does not support getClearValue");
    }
    descriptorSet& getDescriptorSet() override {
        throw std::logic_error("OpenGL backend does not support getDescriptorSet");
    }
    pipelineLayout& getPipelineLayout() override {
        throw std::logic_error("OpenGL backend does not support getPipelineLayout");
    }
    pipeline& getPipeline() override {
        throw std::logic_error("OpenGL backend does not support getPipeline");
    }
    uniformBuffer& getHasTextureBuffer() override {
        throw std::logic_error("OpenGL backend does not support getHasTextureBuffer");
    }
    descriptorSetLayout& getDescriptorSetLayout() override {
        throw std::logic_error("OpenGL backend does not support getDescriptorSetLayout");
    }
    const easyVulkan::renderPassWithFramebuffers& RenderPassAndFramebuffers() override {
        throw std::logic_error("OpenGL backend does not support RenderPassAndFramebuffers");
    }

private:

};


#endif //TOY_RENDERER_UPDATE_SHADEROPENGL_H
