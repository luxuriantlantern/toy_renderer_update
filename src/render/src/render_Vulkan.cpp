//
// Created by 18067 on 25-4-24.
//

#include "render/render_Vulkan.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_vulkan.h>

void Render_Vulkan::init() {
    mShaders[SHADER_TYPE::Blinn_Phong] = std::make_shared<shaderVulkan>(
            "./assets/shaders/Blinn-Phong_v.vert",
            "./assets/shaders/Blinn-Phong_v.frag"
    );
    mShaders[SHADER_TYPE::MATERIAL] = std::make_shared<shaderVulkan>(
            "./assets/shaders/material_v.vert",
            "./assets/shaders/material_v.frag"
    );
    mShaders[SHADER_TYPE::WIREFRAME] = std::make_shared<shaderVulkan>(
            "./assets/shaders/wireframe_v.vert",
            "./assets/shaders/wireframe_v.frag"
    );
    mShaders[SHADER_TYPE::Blinn_Phong]->setShaderType(SHADER_TYPE::Blinn_Phong);
    mShaders[SHADER_TYPE::MATERIAL]->setShaderType(SHADER_TYPE::MATERIAL);
    mShaders[SHADER_TYPE::WIREFRAME]->setShaderType(SHADER_TYPE::WIREFRAME);

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
        resources.descriptorSets.clear();
        for (auto& pool : resources.descriptorPools) {
            pool.~descriptorPool();
        }
        resources.descriptorPools.clear();
        for (auto& sampler : resources.samplers) {
            sampler.~sampler();
        }
        resources.samplers.clear();
        for (auto& tex : resources.textures) {
            tex.~texture2d();
        }
        resources.textures.clear();
        for (auto& buffer : resources.uniformBuffers) {
            buffer.~uniformBuffer();
        }
        resources.uniformBuffers.clear();
        for (auto& buffer : resources.hasTextureBuffers) {
            buffer.~uniformBuffer();
        }
        resources.hasTextureBuffers.clear();
        for (auto& buffer : resources.vertexBuffers_Material) {
            buffer.Destroy();
        }
        resources.vertexBuffers_Material.clear();
    }
    mModelResources.clear();

}

Render_Vulkan::~Render_Vulkan() {
    cleanup();
}

void Render_Vulkan::addModel(const std::shared_ptr<Object>& model) {
    VulkanModelResources &resources = mModelResources[model];

    size_t shapeCount = model->getShapeCount();
    for (size_t i = 0; i < shapeCount; ++i) {
        const std::vector<glm::vec3>& vertices = model->getVertices(i);
        const std::vector<glm::vec3>& normals = model->getNormals(i);
        const std::vector<glm::vec2>& texCoords = model->getTexCoords(i);
        const std::string& texturePath = !model->getTexturePath(i).empty() ? model->getTexturePath(i) : "./assets/textures/white.png";


        resources.uniformBuffers.emplace_back(sizeof(shaderVulkan::uniformBufferObject));
        resources.hasTextureBuffers.emplace_back(sizeof(int));
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
        };
        resources.descriptorPools.emplace_back(1, poolSizes);
        resources.descriptorSets.emplace_back();
        resources.descriptorPools.back().AllocateSets(resources.descriptorSets.back(), getMaterialShader()->getDescriptorSetLayout());

        VkDescriptorBufferInfo unifromBufferInfo = {
            .buffer = resources.uniformBuffers.back(),
            .offset = 0,
            .range = sizeof(shaderVulkan::uniformBufferObject)
        };
        VkDescriptorBufferInfo intInfo = {
            .buffer = resources.hasTextureBuffers.back(),
            .offset = 0,
            .range = sizeof(int)
        };
        VkWriteDescriptorSet writes[] = {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = resources.descriptorSets.back(),
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &unifromBufferInfo
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = resources.descriptorSets.back(),
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &intInfo
        }
        };
        vkUpdateDescriptorSets(graphicsBase::Base().Device(), 2, writes, 0, nullptr);
        resources.textures.emplace_back(texturePath.c_str(), VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, true);
        VkSamplerCreateInfo samplerInfo = texture::SamplerCreateInfo();
        resources.samplers.emplace_back(samplerInfo);
        VkDescriptorImageInfo imageInfo = resources.textures.back().DescriptorImageInfo(resources.samplers.back());
        resources.descriptorSets.back().Write(imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2);


        std::vector<shaderVulkan::material> buffer;
        for (size_t j = 0; j < vertices.size(); ++j) {
            glm::vec2 tex;
            if(!texCoords.empty()) {
                tex = glm::vec2{texCoords[j].x, 1.0f - texCoords[j].y};
            }
            else {
                tex = glm::vec2{0.0f, 0.0f};
            }
            buffer.push_back({vertices[j], normals[j], tex});
        }
        resources.vertexBuffers_Material.emplace_back((buffer.size() + 1) * sizeof(shaderVulkan::material));
        resources.vertexBuffers_Material.back().TransferData(buffer.data(), buffer.size() * sizeof(shaderVulkan::material));
        resources.vertexCounts.push_back(static_cast<uint32_t>(vertices.size()));
    }

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
        for (auto& buffer : resources.vertexBuffers_Material) {
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

    commandBuffer &CommandBuffer = shader->getCommandBuffer().value();

    CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkClearValue clearValues[2];
    std::memcpy(clearValues, shader->getClearValue(), sizeof(clearValues));
    auto &rpwf = shader->RenderPassAndFramebuffers();
    rpwf.pass.CmdBegin(CommandBuffer, rpwf.framebuffers[i], {{}, windowSize}, clearValues);

    for (const auto& model : models) {
        shaderVulkan::uniformBufferObject ubo{};
        ubo.model = model->getModelMatrix();
        ubo.view = viewMatrix;
        ubo.proj = projectionMatrix;

        shader->getUniformBuffer().TransferData(&ubo, sizeof(ubo));
//      TODO: Finish material
        for(size_t idx = 0; idx < mModelResources[model].vertexCounts.size(); ++idx) {
            mModelResources[model].uniformBuffers[idx].TransferData(&ubo, sizeof(ubo));
            VkDeviceSize offset = 0;
            if(shader->getShaderType() == SHADER_TYPE::Blinn_Phong || shader->getShaderType() == SHADER_TYPE::WIREFRAME)
                vkCmdBindVertexBuffers(CommandBuffer, 0, 1, mModelResources[model].vertexBuffers_Material[idx].Address(), &offset);
            else if(shader->getShaderType() == SHADER_TYPE::MATERIAL)
                vkCmdBindVertexBuffers(CommandBuffer, 0, 1, mModelResources[model].vertexBuffers_Material[idx].Address(), &offset);
            vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->getPipeline());

            if(shader->getShaderType() == SHADER_TYPE::MATERIAL && !model->getTexCoords(idx).empty())
            {
                uint32_t hasTexture = model->getTexCoords(idx).size();
                mModelResources[model].hasTextureBuffers[idx].TransferData(&hasTexture, sizeof(uint32_t));
                vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        shader->getPipelineLayout(), 0, 1, mModelResources[model].descriptorSets[idx].Address(), 0,
                                        nullptr);
            }
            else
                vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        shader->getPipelineLayout(), 0, 1, shader->getDescriptorSet().Address(), 0,
                                        nullptr);
            vkCmdDraw(CommandBuffer, mModelResources[model].vertexCounts[idx], 1, 0, 0);
        }
    }
}