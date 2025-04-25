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
    mShaders[SHADER_TYPE::Blinn_Phong]->setShaderType(SHADER_TYPE::Blinn_Phong);
    for(auto & shader : mShaders)
    {
        shader.second->init();
    }
    mCurrentShader = { SHADER_TYPE::Blinn_Phong, mShaders[SHADER_TYPE::Blinn_Phong] };
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

    // 假设 Object 有 getShapes()，每个 shape 有 getVertices()、getIndices() 等
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

            vertexBuffer vertexBuffer(buffer.size() * sizeof(shaderVulkan::material));
            vertexBuffer.TransferData(buffer);

            resources.vertexBuffers.push_back(std::move(vertexBuffer));
            resources.vertexCounts.push_back(static_cast<uint32_t>(vertices.size()));
        }
        else
        {
            std::vector<shaderVulkan::vertex> buffer;
            for (size_t j = 0; j < vertices.size(); ++j) {
                buffer.push_back({vertices[j], normals[j]});
            }

            vertexBuffer vertexBuffer(buffer.size() * sizeof(shaderVulkan::vertex));
            vertexBuffer.TransferData(buffer);

            resources.vertexBuffers.push_back(std::move(vertexBuffer));
            resources.vertexCounts.push_back(static_cast<uint32_t>(vertices.size()));
        }
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
    shader->use();

    auto models = scene->getModels();
    for (const auto& model : models)
    {
        shaderVulkan::uniformBufferObject ubo{};
        ubo.model = model->getModelMatrix();
        ubo.view = viewMatrix;
        ubo.proj = projectionMatrix;
        ubo.proj[1][1] *= -1;

        shader->getUniformBuffer().TransferData(&ubo, sizeof(ubo));

        graphicsBase::Base().SwapImage(shader->getSemaphoreImageIsAvailable());
        auto i = graphicsBase::Base().CurrentImageIndex();

        commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        renderPass.CmdBegin(commandBuffer, framebuffers[i], { {}, windowSize }, clearColor);
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.Address(), &offset);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_triangle);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout_triangle, 0, 1, descriptorSet_triangle.Address(), 0, nullptr);
        vkCmdDraw(commandBuffer, 36, 1, 0, 0);
        renderPass.CmdEnd(commandBuffer);
        commandBuffer.End();

        graphicsBase::Base().SubmitCommandBuffer_Graphics(commandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);
        graphicsBase::Base().PresentImage(semaphore_renderingIsOver);
    }
}