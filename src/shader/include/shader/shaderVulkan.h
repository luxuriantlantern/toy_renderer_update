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

class shaderVulkan : public Shader {
public:
    shaderVulkan(std::string vertexPath, std::string fragmentPath, std::string geometryPath = "") : Shader()
    {
        mVertexPath = vertexPath;
        mFragmentPath = fragmentPath;
        mGeometryPath = geometryPath;
        mBackendType = VULKAN;
        LoadShaders(vertexPath, fragmentPath);
    }
    std::vector<uint32_t> CompileGLSLToSPIRV(const std::string& shaderSource, EShLanguage stage);
    std::string readFile(const std::string& filepath);
    void LoadShaders(const std::string &vertexPath, const std::string &fragmentPath, const std::string &geometryPath = "");
    void use() override;
    void init() override;
    void setCommandBuffer(VkCommandBuffer cmdBuffer) { currentCommandBuffer = cmdBuffer; }

private:
    static shaderModule vert, frag, geom;
    VkPipelineShaderStageCreateInfo shaderStages[3];
    uint32_t stageCount = 0;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkCommandBuffer currentCommandBuffer = VK_NULL_HANDLE;
};

#endif //TOY_RENDERER_UPDATE_SHADERVULKAN_H
