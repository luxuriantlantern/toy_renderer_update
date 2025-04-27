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
    void setShaderType(SHADER_TYPE type){mShaderType = type;}
    std::vector<uint32_t> CompileGLSLToSPIRV(const std::string& shaderSource, EShLanguage stage);
    std::string readFile(const std::string& filepath);
    void LoadShaders(const std::string &vertexPath, const std::string &fragmentPath, const std::string &geometryPath = "");
    void use() override;
    void init() override;

    const auto& RenderPassAndFramebuffers() {
        static const auto& rpwf = easyVulkan::CreateRpwf_Screen();
        return rpwf;
    }

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

    void cleanup() override;
    void setBool(const std::string &name, bool value) const override;
    void setInt(const std::string &name, int value) const override;
    void setFloat(const std::string &name, float value) const override;
    void setVec3(const std::string &name, const glm::vec3 &value) const override;
    void setMat4(const std::string &name, const glm::mat4 &mat) const override;

    semaphore& getSemaphoreImageIsAvailable() override { return msemaphore_imageIsAvailable; }
    uniformBuffer& getUniformBuffer() override { return muniformBuffer; }
    commandBuffer& getCommandBuffer() override { return mcommandBuffer; }
    fence& getFence() override { return mfence; }
    semaphore& getSemaphoreRenderingIsOver() override { return msemaphore_renderingIsOver; }
    VkClearValue& getClearValue() override { return mclearColor; }
    descriptorSet& getDescriptorSet() override { return mdescriptorSet_triangle; }
    pipelineLayout& getPipelineLayout() override { return pipelineLayout_triangle; }
    pipeline& getPipeline() override { return pipeline_triangle; }

private:
    shaderModule vert, frag, geom;
    uint32_t stageCount = 0;
    descriptorSetLayout descriptorSetLayout_triangle;
    pipelineLayout pipelineLayout_triangle;
    pipeline pipeline_triangle;

    uniformBuffer muniformBuffer = std::move(uniformBuffer(sizeof(uniformBufferObject)));
    descriptorPool mdescriptorPool = std::move(descriptorPool(1, VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }));
    descriptorSet mdescriptorSet_triangle;
    std::unique_ptr<VkDescriptorBufferInfo> mbufferInfo = std::make_unique<VkDescriptorBufferInfo>(VkDescriptorBufferInfo{VK_NULL_HANDLE, 0, 0});

    std::unique_ptr<VkWriteDescriptorSet> mdescriptorWrite = std::make_unique<VkWriteDescriptorSet>(VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, VK_NULL_HANDLE, 0, 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            nullptr,
            nullptr,
            nullptr
    });
    VkClearValue mclearColor = VkClearValue{ .color = { 0.f, 0.f, 0.f, 1.f } };
    void initForUniform();

    fence mfence;
    semaphore msemaphore_imageIsAvailable;
    semaphore msemaphore_renderingIsOver;
    commandBuffer mcommandBuffer;
    commandPool mcommandPool = commandPool(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
};

#endif //TOY_RENDERER_UPDATE_SHADERVULKAN_H
