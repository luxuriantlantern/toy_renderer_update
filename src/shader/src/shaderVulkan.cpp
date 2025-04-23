//
// Created by ftc on 25-4-23.
//

#include "shader/shaderVulkan.h"

std::vector<uint32_t> shaderVulkan::CompileGLSLToSPIRV(const std::string& shaderSource, EShLanguage stage) {
    glslang::TShader shader(stage);
    const char* source = shaderSource.c_str();
    shader.setStrings(&source, 1);

    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(GetDefaultResources(), 100, false, messages)) {
        std::cerr << "Shader compilation failed:\n" << shader.getInfoLog() << std::endl;
        return {};
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        std::cerr << "Shader linking failed:\n" << program.getInfoLog() << std::endl;
        return {};
    }

    std::vector<uint32_t> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
    return spirv;
}

std::string shaderVulkan::readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    return content;
}

void shaderVulkan::LoadShaders(const std::string &vertexPath, const std::string &fragmentPath, const std::string &geometryPath)
{
    glslang::InitializeProcess();

    auto vertCode = readFile(vertexPath);
    auto fragCode = readFile(fragmentPath);

    std::vector<uint32_t> vertSpirv = CompileGLSLToSPIRV(vertCode, EShLangVertex);
    std::vector<uint32_t> fragSpirv = CompileGLSLToSPIRV(fragCode, EShLangFragment);

    graphicsBase::Base().WaitIdle();
    if(vert)vert.~shaderModule();
    if(frag)frag.~shaderModule();

    vert.Create(vertSpirv.size() * sizeof(uint32_t), vertSpirv.data());
    frag.Create(fragSpirv.size() * sizeof(uint32_t), fragSpirv.data());

    shaderStages[0] = vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT);
    stageCount = 2;

    glslang::FinalizeProcess();
}

void shaderVulkan::use() {
    if(mBackendType != SHADER_BACKEND_TYPE::VULKAN)return;
    if(!pipeline) {
        std::cerr << "Shader program not initialized!" << std::endl;
        return;
    }
    vkCmdBindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}