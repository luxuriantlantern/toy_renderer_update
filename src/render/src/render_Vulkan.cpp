//
// Created by 18067 on 25-4-24.
//

#include "render/render_Vulkan.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>

void Render_Vulkan::init() {
    mShaders[SHADER_TYPE::Blinn_Phong] = std::make_shared<shaderVulkan>(
            "./assets/shaders/Blinn-Phong_v.vert",
            "./assets/shaders/Blinn-Phong_v.frag"
    );
    mShaders[SHADER_TYPE::MATERIAL] = std::make_shared<shaderVulkan>(
            "./assets/shaders/material_v.vert",
            "./assets/shaders/material_v.frag"
    );
    mShaders[SHADER_TYPE::Blinn_Phong]->setShaderType(SHADER_TYPE::Blinn_Phong);
    mShaders[SHADER_TYPE::MATERIAL]->setShaderType(SHADER_TYPE::MATERIAL);
    for(auto & shader : mShaders)
    {
        shader.second->init();
    }
    mCurrentShader = { SHADER_TYPE::MATERIAL, mShaders[SHADER_TYPE::MATERIAL] };
}

void Render_Vulkan::cleanup() {
    if(getType() != VULKAN) return;
    graphicsBase::Base().WaitIdle();

    for (auto& model : mModelResources) {
        auto& resources = model.second;
        for (auto& buffer : resources.vertexBuffers) {
            buffer.Destroy();
        }
    }
    mModelResources.clear();
}

Render_Vulkan::~Render_Vulkan() {
    cleanup();
}

void Render_Vulkan::addModel(const std::shared_ptr<Object>& model) {
    VulkanModelResources resources;

    size_t shapeCount = model->getShapeCount();
    for (size_t i = 0; i < shapeCount; ++i) {
        const std::vector<glm::vec3>& vertices = model->getVertices(i);
        const std::vector<glm::vec3>& normals = model->getNormals(i);
        const std::vector<glm::vec2>& texCoords = model->getTexCoords(i);
        const std::string& texturePath = model->getTexturePath(i);

        if(!texCoords.empty())
        {
            std::vector<shaderVulkan::material> buffer;
            for (size_t j = 0; j < vertices.size(); ++j) {
                buffer.push_back({vertices[j], normals[j], texCoords[j]});
            }

            resources.vertexBuffers_Material.emplace_back(buffer.size() * sizeof(shaderVulkan::material));
            resources.vertexBuffers_Material.back().TransferData(buffer.data(), buffer.size() * sizeof(shaderVulkan::material));
        }
        std::vector<shaderVulkan::vertex> buffer;
        for (size_t j = 0; j < vertices.size(); ++j) {
            buffer.push_back({vertices[j], normals[j]});
        }

        resources.vertexBuffers.emplace_back(buffer.size() * sizeof(shaderVulkan::vertex));
        resources.vertexBuffers.back().TransferData(buffer.data(), buffer.size() * sizeof(shaderVulkan::vertex));
        resources.vertexCounts.push_back(static_cast<uint32_t>(vertices.size()));
//  TODO: add 2d texture
    }

    mModelResources[model] = std::move(resources);
}

void Render_Vulkan::setup(const std::shared_ptr<Scene> &scene) {
    cleanup();
    for (const auto& model : scene->getModels()) {
        addModel(model);
    }
}

void Render_Vulkan::removeModel(const std::shared_ptr<Object>& model)
{
    auto it = mModelResources.find(model);
    if (it != mModelResources.end()) {
        auto& resources = it->second;
        for (auto& buffer : resources.vertexBuffers) {
            buffer.Destroy();
        }
        mModelResources.erase(it);
    }
}

void Render_Vulkan::render(const std::shared_ptr<Scene>& scene, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    auto shader = mCurrentShader.second;
    auto models = scene->getModels();

    graphicsBase::Base().SwapImage(shader->getSemaphoreImageIsAvailable());
    auto i = graphicsBase::Base().CurrentImageIndex();

    commandBuffer &CommandBuffer = shader->getCommandBuffer();
    fence &Fence = shader->getFence();
    semaphore &semaphore_imageIsAvailable = shader->getSemaphoreImageIsAvailable();
    semaphore &semaphore_renderingIsOver = shader->getSemaphoreRenderingIsOver();

    CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkClearValue clearValues[2];
    std::memcpy(clearValues, shader->getClearValue(), sizeof(clearValues));
    rpwf.pass.CmdBegin(CommandBuffer, rpwf.framebuffers[i], {{}, windowSize}, clearValues);

    for (const auto& model : models) {
        shaderVulkan::uniformBufferObject ubo{};
        ubo.model = model->getModelMatrix();
        ubo.view = viewMatrix;
        ubo.proj = projectionMatrix;
        for(int j = 0; j < 4; ++j) ubo.proj[j][1] *= -1;

        shader->getUniformBuffer().TransferData(&ubo, sizeof(ubo));
//      TODO: Finish material
        for(size_t idx = 0; idx < mModelResources[model].vertexBuffers.size(); ++idx) {
            VkDeviceSize offset = 0;
            if(shader->getShaderType() == SHADER_TYPE::Blinn_Phong)
                vkCmdBindVertexBuffers(CommandBuffer, 0, 1, mModelResources[model].vertexBuffers[idx].Address(), &offset);
            else if(shader->getShaderType() == SHADER_TYPE::MATERIAL)
                vkCmdBindVertexBuffers(CommandBuffer, 0, 1, mModelResources[model].vertexBuffers_Material[idx].Address(), &offset);
            vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->getPipeline());
            if(shader->getShaderType() == SHADER_TYPE::MATERIAL)
            {
                uint32_t hasTexture = model->getTexCoords(idx).size();
                shader->getHasTextureBuffer().TransferData(&hasTexture, sizeof(uint32_t));
                if(hasTexture)
                {
                    texture2d texture(model->getTexturePath(idx).c_str(), VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, true);
                    VkSamplerCreateInfo samplerInfo = texture::SamplerCreateInfo();
                    sampler msampler(samplerInfo);
                    VkDescriptorImageInfo imageInfo = texture.DescriptorImageInfo(msampler);

                    VkWriteDescriptorSet write = {};
                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write.dstSet = shader->getDescriptorSet();
                    write.dstBinding = 1;
                    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    write.descriptorCount = 1;
                    write.pImageInfo = &imageInfo;

                    vkUpdateDescriptorSets(graphicsBase::Base().Device(), 1, &write, 0, nullptr);
                }
            }
            vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    shader->getPipelineLayout(), 0, 1, shader->getDescriptorSet().Address(), 0,
                                    nullptr);
            vkCmdDraw(CommandBuffer, mModelResources[model].vertexCounts[idx], 1, 0, 0);
        }
    }

    rpwf.pass.CmdEnd(CommandBuffer);
    CommandBuffer.End();

    graphicsBase::Base().SubmitCommandBuffer_Graphics(CommandBuffer, semaphore_imageIsAvailable,
                                                      semaphore_renderingIsOver, Fence);
    graphicsBase::Base().PresentImage(semaphore_renderingIsOver);
    Fence.WaitAndReset();
}