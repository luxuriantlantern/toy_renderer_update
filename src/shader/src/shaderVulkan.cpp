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
    mcommandBuffer.emplace();
    mcommandPool.emplace(commandPool(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT));
    LoadShaders(mVertexPath, mFragmentPath, mGeometryPath);

    VkDescriptorSetLayoutBinding bindings[3] = {
            { .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
            { .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
            { .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT }
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo_triangle = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    };

    descriptorSetLayoutCreateInfo_triangle.bindingCount = 3, descriptorSetLayoutCreateInfo_triangle.pBindings = bindings;

    descriptorSetLayout_triangle.Create(descriptorSetLayoutCreateInfo_triangle);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = descriptorSetLayout_triangle.Address()
    };
    pipelineLayout_triangle.Create(pipelineLayoutCreateInfo);

    VkPipelineShaderStageCreateInfo shaderStageCreateInfos_triangle[2] = {
            vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
            frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    auto Create = [this, &shaderStageCreateInfos_triangle] {
        graphicsPipelineCreateInfoPack pipelineCiPack;
        pipelineCiPack.createInfo.layout = pipelineLayout_triangle;
        pipelineCiPack.createInfo.renderPass = rpwf.value().value().get().pass;


        pipelineCiPack.vertexInputBindings.emplace_back(0, sizeof(material), VK_VERTEX_INPUT_RATE_VERTEX);
        pipelineCiPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(material, position));
        pipelineCiPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(material, normal));
        pipelineCiPack.vertexInputAttributes.emplace_back(2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(material, tex));

        pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
        pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);

        pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // pipelineCiPack.rasterizationStateCi.cullMode = VK_CULL_MODE_BACK_BIT;
        // pipelineCiPack.rasterizationStateCi.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        if(mShaderType == SHADER_TYPE::WIREFRAME)
        {
            pipelineCiPack.rasterizationStateCi.rasterizerDiscardEnable = VK_FALSE;
            pipelineCiPack.rasterizationStateCi.depthClampEnable = VK_FALSE;
            pipelineCiPack.rasterizationStateCi.depthBiasEnable = VK_FALSE;
            pipelineCiPack.rasterizationStateCi.polygonMode = VK_POLYGON_MODE_LINE;
            pipelineCiPack.rasterizationStateCi.lineWidth = 1.0f;
            pipelineCiPack.rasterizationStateCi.cullMode = VK_CULL_MODE_NONE;
            pipelineCiPack.rasterizationStateCi.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }

        else
        {
            pipelineCiPack.rasterizationStateCi.polygonMode = VK_POLYGON_MODE_FILL;
            pipelineCiPack.rasterizationStateCi.lineWidth = 1.0f;
        }

        pipelineCiPack.depthStencilStateCi.depthTestEnable = VK_TRUE;
        pipelineCiPack.depthStencilStateCi.depthWriteEnable = VK_TRUE;
        pipelineCiPack.depthStencilStateCi.depthCompareOp = VK_COMPARE_OP_LESS;

        pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });

        pipelineCiPack.UpdateAllArrays();
        pipelineCiPack.createInfo.stageCount = 2;
        pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_triangle;

        pipeline_triangle.Create(pipelineCiPack);
    };

    auto Destroy = [this] {
        pipeline_triangle.~pipeline();
    };

    graphicsBase::Base().AddCallback_CreateSwapchain(Create);
    graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
    Create();

    mcommandPool.value().AllocateBuffers(mcommandBuffer.value());
    initForUniform();
}

void shaderVulkan::initForUniform()
{
    muniformBuffer.emplace((sizeof(uniformBufferObject)));
    mHasTextureBuffer.emplace((sizeof(int)));
    VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    };
    mdescriptorPool.emplace(1, poolSizes);
    mdescriptorPool->AllocateSets(mdescriptorSet_triangle, descriptorSetLayout_triangle);
    VkDescriptorBufferInfo unifromBufferInfo = {
            .buffer = *muniformBuffer,
            .offset = 0,
            .range = sizeof(uniformBufferObject)
    };
    VkDescriptorBufferInfo intInfo = {
            .buffer = *mHasTextureBuffer,
            .offset = 0,
            .range = sizeof(int)
    };

    VkWriteDescriptorSet writes[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = mdescriptorSet_triangle,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &unifromBufferInfo
            },
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = mdescriptorSet_triangle,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &intInfo
            }
    };
    vkUpdateDescriptorSets(graphicsBase::Base().Device(), 2, writes, 0, nullptr);

    if(mShaderType == SHADER_TYPE::Blinn_Phong || mShaderType == SHADER_TYPE::WIREFRAME)
    {
        int dummpyHasTexture = 0;
        mHasTextureBuffer->TransferData(&dummpyHasTexture, sizeof(int));
    }

    dummyTexture.emplace("assets/textures/testImage.png", VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, true);
    VkSamplerCreateInfo samplerInfo = texture::SamplerCreateInfo();
    msampler.emplace(samplerInfo);
    VkDescriptorImageInfo imageInfo = dummyTexture->DescriptorImageInfo(*msampler);
    mdescriptorSet_triangle.Write(imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2);
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
    if (mBackendType != VULKAN) return;
    graphicsBase::Base().WaitIdle();
    if (pipelineLayout_triangle) {
        pipelineLayout_triangle.~pipelineLayout();
    }
    if (descriptorSetLayout_triangle) {
        descriptorSetLayout_triangle.~descriptorSetLayout();
    }
    if (mdescriptorPool) {
        mdescriptorPool->~descriptorPool();
        mdescriptorPool.reset();
    }
    if (msampler) {
        msampler->~sampler();
        msampler.reset();
    }
    if (dummyTexture) {
        dummyTexture->~texture2d();
        dummyTexture.reset();
    }
    if (muniformBuffer) {
        muniformBuffer->~uniformBuffer();
        muniformBuffer.reset();
    }
    if (mHasTextureBuffer) {
        mHasTextureBuffer->~uniformBuffer();
        mHasTextureBuffer.reset();
    }
    if (mcommandPool)
    {
        mcommandPool->~commandPool();
        mcommandPool.reset();
    }
    if (vert) {
        vert.~shaderModule();
    }
    if (frag) {
        frag.~shaderModule();
    }
    if (mfence) {
        mfence.~fence();
    }
    if (msemaphore_imageIsAvailable) {
        msemaphore_imageIsAvailable.~semaphore();
    }
    if (msemaphore_renderingIsOver) {
        msemaphore_renderingIsOver.~semaphore();
    }
}