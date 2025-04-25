//
// Created by 18067 on 25-4-24.
//

#include "render/render_Vulkan.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>

static const auto&[RenderPass, Framebuffers] = easyVulkan::CreateRpwf_Screen();


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
    size_t idx = 0;
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

        commandBuffer CommandBuffer = std::move(shader->getCommandBuffer());

        fence Fence = std::move(shader->getFence());
        semaphore semaphore_imageIsAvailable = std::move(shader->getSemaphoreImageIsAvailable());
        semaphore semaphore_renderingIsOver = std::move(shader->getSemaphoreRenderingIsOver());

        CommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        RenderPass.CmdBegin(CommandBuffer, Framebuffers[i], { {}, windowSize }, shader->getClearValue());
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, mModelResources[model].vertexBuffers[idx].Address(), &offset);
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->getPipeline());
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                shader->getPipelineLayout(), 0, 1, shader->getDescriptorSet().Address(), 0, nullptr);
        vkCmdDraw(CommandBuffer, mModelResources[model].vertexCounts[idx], 1, 0, 0);
        RenderPass.CmdEnd(CommandBuffer);
        CommandBuffer.End();

        graphicsBase::Base().SubmitCommandBuffer_Graphics(CommandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, shader->getFence());
        graphicsBase::Base().PresentImage(semaphore_renderingIsOver);
        shader->getFence().WaitAndReset();
        idx ++;
    }
}