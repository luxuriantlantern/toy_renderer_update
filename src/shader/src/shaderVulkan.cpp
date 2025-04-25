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

    stageCount = 2;

    glslang::FinalizeProcess();
}

void shaderVulkan::init()
{
    VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo_triangle = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &binding
    };
    descriptorSetLayout_triangle.Create(descriptorSetLayoutCreateInfo_triangle);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = descriptorSetLayout_triangle.Address()
    };
    pipelineLayout_triangle.Create(pipelineLayoutCreateInfo);

    static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_triangle[2] = {
            vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
            frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    auto Create = [this] {
        graphicsPipelineCreateInfoPack pipelineCiPack;
        pipelineCiPack.createInfo.layout = pipelineLayout_triangle;
        pipelineCiPack.createInfo.renderPass = RenderPassAndFramebuffers().pass;

        if(mShaderType == SHADER_TYPE::Blinn_Phong || mShaderType == SHADER_TYPE::WIREFRAME)
        {
            pipelineCiPack.vertexInputBindings.emplace_back(0, sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX);
            pipelineCiPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, position));
            pipelineCiPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, normal));
        }

        if(mShaderType == SHADER_TYPE::MATERIAL)
        {
            pipelineCiPack.vertexInputBindings.emplace_back(0, sizeof(material), VK_VERTEX_INPUT_RATE_VERTEX);
            pipelineCiPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(material, position));
            pipelineCiPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(material, normal));
            pipelineCiPack.vertexInputAttributes.emplace_back(2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(material, tex));
        }

        pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
        pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
        pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });

        pipelineCiPack.UpdateAllArrays();
        pipelineCiPack.createInfo.stageCount = 2;
        pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_triangle;

        pipeline_triangle.Create(pipelineCiPack);
    };

    auto Destroy = [this] { pipeline_triangle.~pipeline(); };

    graphicsBase::Base().AddCallback_CreateSwapchain(Create);
    graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
    Create();

    commandPool commandPool(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandPool.AllocateBuffers(mcommandBuffer);


}

void shaderVulkan::initForUniform()
{
    mdescriptorPool.AllocateSets(mdescriptorSet_triangle, descriptorSetLayout_triangle);
    mbufferInfo = std::make_unique<VkDescriptorBufferInfo>(VkDescriptorBufferInfo{
            .buffer = muniformBuffer,
            .offset = 0,
            .range = sizeof(uniformBufferObject)
    });
    mdescriptorWrite = std::make_unique<VkWriteDescriptorSet>(VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mdescriptorSet_triangle,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = mbufferInfo.get()
    });
}

void shaderVulkan::use() {
    vkUpdateDescriptorSets(graphicsBase::Base().Device(), 1, mdescriptorWrite.get(), 0, nullptr);
}

void shaderVulkan::setBool(const std::string &name, bool value) const {
    (void)name;
    (void)value;
}

void shaderVulkan::setInt(const std::string &name, int value) const {
    (void)name;
    (void)value;
}

void shaderVulkan::setFloat(const std::string &name, float value) const {
    (void)name;
    (void)value;
}

void shaderVulkan::setVec3(const std::string &name, const glm::vec3 &value) const {
    (void)name;
    (void)value;
}

void shaderVulkan::setMat4(const std::string &name, const glm::mat4 &mat) const {
    (void)name;
    (void)mat;
}

void shaderVulkan::cleanup() {
    if(mBackendType != VULKAN) return;
    graphicsBase::Base().WaitIdle();

    mfence.~fence();
    msemaphore_imageIsAvailable.~semaphore();
    msemaphore_renderingIsOver.~semaphore();

    pipeline_triangle.~pipeline();
    pipelineLayout_triangle.~pipelineLayout();
    descriptorSetLayout_triangle.~descriptorSetLayout();

    mdescriptorPool.~descriptorPool();

    muniformBuffer.~uniformBuffer();

    vert.~shaderModule();
    frag.~shaderModule();
}