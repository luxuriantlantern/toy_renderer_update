//
// Created by ftc on 25-4-23.
//

#ifndef TOY_RENDERER_UPDATE_SHADERVULKAN_H
#define TOY_RENDERER_UPDATE_SHADERVULKAN_H

#include "shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include "EasyVulkan/GlfwGeneral.hpp"
#include <glslang/glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/StandAlone/DirStackFileIncluder.h>
#include <glslang/glslang/Public/ResourceLimits.h>
#include <iostream>

class shaderVulkan : public Shader {
public:
    shaderVulkan(std::string vertexPath, std::string fragmentPath, std::string geometryPath = "") : Shader()
    {
        mVertexPath = vertexPath;
        mFragmentPath = fragmentPath;
        mGeometryPath = geometryPath;
        mBackendType = VULKAN;
        mProgram = 0;
    }
    ~shaderVulkan()
    {
        cleanup();
    }
    std::vector<uint32_t> CompileGLSLToSPIRV(const std::string& shaderSource, EShLanguage stage);
    std::string readFile(const std::string& filepath);
    void LoadShaders(const std::string &vertexPath, const std::string &fragmentPath, const std::string &geometryPath = "");
    void use() override  {return;}
    void init() override;



    struct vertex {
        glm::vec3 position;
        glm::vec3 normal;
    };

    struct material {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 tex;
    };

    struct uniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct unoformBufferObject_Material {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;

    };

    void cleanup() override;
    void setBool(const std::string &name, bool value) const override;
    void setInt(const std::string &name, int value) const override;
    void setFloat(const std::string &name, float value) const override;
    void setVec3(const std::string &name, const glm::vec3 &value) const override;
    void setMat4(const std::string &name, const glm::mat4 &mat) const override;

    semaphore& getSemaphoreImageIsAvailable() override { return msemaphore_imageIsAvailable; }
    uniformBuffer& getUniformBuffer() override { return *muniformBuffer; }
    uniformBuffer& getHasTextureBuffer() override { return *mHasTextureBuffer; }
    std::optional<commandBuffer> & getCommandBuffer() override { return mcommandBuffer; }
    fence& getFence() override { return mfence; }
    semaphore& getSemaphoreRenderingIsOver() override { return msemaphore_renderingIsOver; }
    VkClearValue* getClearValue() override { return mclearValue; }
    descriptorSet& getDescriptorSet() override { return mdescriptorSet_triangle; }
    pipelineLayout& getPipelineLayout() override { return pipelineLayout_triangle; }
    pipeline& getPipeline() override { return pipeline_triangle; }
    descriptorSetLayout& getDescriptorSetLayout() override { return descriptorSetLayout_triangle; }

     const easyVulkan::renderPassWithFramebuffers& RenderPassAndFramebuffers() override {
        static const auto& rpwf = easyVulkan::CreateRpwf_ScreenWithDS();
        return rpwf;
    }

private:
    shaderModule vert, frag, geom;
    uint32_t stageCount = 0;
    descriptorSetLayout descriptorSetLayout_triangle;
    pipelineLayout pipelineLayout_triangle;
    pipeline pipeline_triangle;

    std::optional<uniformBuffer> muniformBuffer;
    std::optional<uniformBuffer> mDummyBuffer;
    std::optional<uniformBuffer> mHasTextureBuffer;
    std::optional<descriptorPool> mdescriptorPool;

    descriptorSet mdescriptorSet_triangle;

    VkClearValue mclearValue[2] = {
            { .color = { 0.f, 0.f, 0.f, 1.f } },
            { .depthStencil = { 1.f, 0 } }
    };
    void initForUniform();
    std::optional<texture2d> dummyTexture;
    std::optional<sampler> msampler;

    fence mfence;
    semaphore msemaphore_imageIsAvailable;
    semaphore msemaphore_renderingIsOver;
    std::optional<commandBuffer> mcommandBuffer;
    std::optional<commandPool> mcommandPool;
};

#endif //TOY_RENDERER_UPDATE_SHADERVULKAN_H
