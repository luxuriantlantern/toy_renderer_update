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
#include "EasyVulkan/easyVulkan.h"
#include <glslang/glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/StandAlone/DirStackFileIncluder.h>
#include <glslang/glslang/Public/ResourceLimits.h>
#include <iostream>

class shaderVulkan : public Shader {
public:
    shaderVulkan(std::string vertexPath, std::string fragmentPath, std::string geometryPath = "", SHADER_TYPE shadertype = SHADER_TYPE::MATERIAL) : Shader()
    {
        mVertexPath = vertexPath;
        mFragmentPath = fragmentPath;
        mGeometryPath = geometryPath;
        mBackendType = VULKAN;
        mShaderType = shadertype;
        LoadShaders(vertexPath, fragmentPath);
        init();
    }
    std::vector<uint32_t> CompileGLSLToSPIRV(const std::string& shaderSource, EShLanguage stage);
    std::string readFile(const std::string& filepath);
    void LoadShaders(const std::string &vertexPath, const std::string &fragmentPath, const std::string &geometryPath = "");
    void use() override;
    void init() override;

    const auto& RenderPassAndFramebuffers() {
        static const auto& rpwf = easyVulkan::CreateRpwf_Screen();
        return rpwf;
    }


private:
    shaderModule vert, frag, geom;
    VkPipelineShaderStageCreateInfo shaderStageCreateInfos_triangle[3];
    uint32_t stageCount = 0;
    descriptorSetLayout descriptorSetLayout_triangle;
    pipelineLayout pipelineLayout_triangle;
    pipeline pipeline_triangle;

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

    uniformBuffer muniformBuffer = uniformBuffer(sizeof(uniformBufferObject));
    VkDescriptorPoolSize mdescriptorPoolSizes[1] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
    };
    descriptorPool mdescriptorPool = descriptorPool(1, mdescriptorPoolSizes);
    descriptorSet mdescriptorSet_triangle;
    VkDescriptorBufferInfo mbufferInfo;
    VkWriteDescriptorSet mdescriptorWrite;
    VkClearValue mclearColor = { .color = { 0.f, 0.f, 0.f, 1.f } };
    void initForUniform();
    renderPass mrenderpass;
    framebuffer mframebuffer;

    fence fence;
    semaphore semaphore_imageIsAvailable;
    semaphore semaphore_renderingIsOver;
    commandBuffer commandBuffer;
};

#endif //TOY_RENDERER_UPDATE_SHADERVULKAN_H
