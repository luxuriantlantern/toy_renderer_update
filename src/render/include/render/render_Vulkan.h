//
// Created by 18067 on 25-4-24.
//

#ifndef RENDER_VULKAN_H
#define RENDER_VULKAN_H

#include "render.h"
#include "shader/shaderVulkan.h"



class Render_Vulkan : public Render{
public:
    Render_Vulkan() = default;
    ~Render_Vulkan() override;
    void setup(const std::shared_ptr<Scene>& scene) override;
    void addModel(const std::shared_ptr<Object>& model) override;
    void removeModel(const std::shared_ptr<Object>& model) override;
    void init() override;
    void render(
        const std::shared_ptr<Scene>& scene,
        const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix
    ) override;
    SHADER_BACKEND_TYPE getType() override { return SHADER_BACKEND_TYPE::OPENGL; }
    void setShaderType(SHADER_TYPE type) override {
        mCurrentShader = { type, mShaders[type] };
    }
    SHADER_TYPE getShaderType() const override {
        return mCurrentShader.first;
    }
private:
    void cleanup() override;
    void loadTexture(const std::string& path, GLuint& textureID);
    struct VulkanModelResources {
        std::vector<vertexBuffer> vertexBuffers;
        std::vector<uint32_t> vertexCounts;
        std::vector<vertexBuffer> vertexBuffers_Material;
//        std::vector<VkDescriptorSet> descriptorSets;
//        std::vector<VkImage> textures;
//        std::vector<VkImageView> textureViews;
//        std::vector<VkSampler> samplers;
    };
    easyVulkan::renderPassWithFramebuffers& rpwf = easyVulkan::CreateRpwf_ScreenWithDS();
    std::unordered_map<std::shared_ptr<Object>, VulkanModelResources> mModelResources;
};

#endif //RENDER_VULKAN_H
