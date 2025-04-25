#include "EasyVulkan/GlfwGeneral.hpp"
#include "EasyVulkan/easyVulkan.h"
#include <glslang/glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/StandAlone/DirStackFileIncluder.h>
#include <glslang/glslang/Public/ResourceLimits.h>
using namespace vulkan;

descriptorSetLayout descriptorSetLayout_triangle;
pipelineLayout pipelineLayout_triangle;
pipeline pipeline_triangle;

const auto& RenderPassAndFramebuffers() {
    static const auto& rpwf = easyVulkan::CreateRpwf_Screen();
    return rpwf;
}

void CreateLayout() {
    // 更新为单个uniform buffer绑定，包含所有矩阵
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
}

void InitGlslang() {
    glslang::InitializeProcess();
}

void FinalizeGlslang() {
    glslang::FinalizeProcess();
}

std::vector<uint32_t> CompileGLSLToSPIRV(const std::string& shaderSource, EShLanguage stage) {
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

std::string readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    return content;
}

struct vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

void CreatePipeline() {
    InitGlslang();

    std::string vertShaderCode = readFile("./assets/shaders/Blinn-Phong_v.vert");
    std::string fragShaderCode = readFile("./assets/shaders/Blinn-Phong_v.frag");

    std::vector<uint32_t> vertSpirv = CompileGLSLToSPIRV(vertShaderCode, EShLangVertex);
    std::vector<uint32_t> fragSpirv = CompileGLSLToSPIRV(fragShaderCode, EShLangFragment);

    static shaderModule vert(vertSpirv.size() * sizeof(uint32_t), vertSpirv.data());
    static shaderModule frag(fragSpirv.size() * sizeof(uint32_t), fragSpirv.data());

    FinalizeGlslang();

    static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_triangle[2] = {
            vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
            frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    auto Create = [] {
        graphicsPipelineCreateInfoPack pipelineCiPack;
        pipelineCiPack.createInfo.layout = pipelineLayout_triangle;
        pipelineCiPack.createInfo.renderPass = RenderPassAndFramebuffers().pass;

        pipelineCiPack.vertexInputBindings.emplace_back(0, sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX);
        pipelineCiPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, position));
        pipelineCiPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, normal));

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

    auto Destroy = [] { pipeline_triangle.~pipeline(); };

    graphicsBase::Base().AddCallback_CreateSwapchain(Create);
    graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
    Create();
}

int main() {
    if (!InitializeWindow({1280, 720}))
        return -1;
    windowSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;
    VkExtent2D prevWindowSize = windowSize;

    const auto& [renderPass, framebuffers] = RenderPassAndFramebuffers();
    CreateLayout();
    CreatePipeline();

    fence fence;
    semaphore semaphore_imageIsAvailable;
    semaphore semaphore_renderingIsOver;

    commandBuffer commandBuffer;
    commandPool commandPool(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandPool.AllocateBuffers(commandBuffer);

    // 顶点数据保持不变
    vertex vertices[] = {
            // Front face
            {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},

            // Back face
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
            {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}},

            // Left face
            {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}},
            {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}},
            {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}},
            {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}},

            // Right face
            {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},

            // Top face
            {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},

            // Bottom face
            {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}},
            {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
            {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}}
    };

    vertexBuffer vertexBuffer(sizeof(vertices));
    vertexBuffer.TransferData(vertices);

    // 定义与着色器匹配的uniform buffer结构
    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };

    // 创建uniform buffer
    uniformBuffer uniformBuffer(sizeof(UniformBufferObject));

    // 创建描述符池和描述符集
    VkDescriptorPoolSize descriptorPoolSizes[] = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
    };
    descriptorPool descriptorPool(1, descriptorPoolSizes);
    descriptorSet descriptorSet_triangle;
    descriptorPool.AllocateSets(descriptorSet_triangle, descriptorSetLayout_triangle);

    // 更新描述符集
    VkDescriptorBufferInfo bufferInfo = {
            .buffer = uniformBuffer,
            .offset = 0,
            .range = sizeof(UniformBufferObject)
    };

    VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet_triangle,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo
    };
    vkUpdateDescriptorSets(graphicsBase::Base().Device(), 1, &descriptorWrite, 0, nullptr);

    VkClearValue clearColor = { .color = { 1.f, 0.f, 0.f, 1.f } };

    while (!glfwWindowShouldClose(pWindow)) {
        while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED))
            glfwWaitEvents();

        int width = 0, height = 0;
        glfwGetFramebufferSize(pWindow, &width, &height);

        if (width > 0 && height > 0 &&
            (width != prevWindowSize.width || height != prevWindowSize.height)) {
            graphicsBase::Base().WaitIdle();
            windowSize.width = width;
            windowSize.height = height;
            if (graphicsBase::Base().RecreateSwapchain()) {
                std::cerr << "Failed to recreate swapchain\n";
                break;
            }
            prevWindowSize = windowSize;
            continue;
        }

        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float>(currentTime - startTime).count();

        // 更新uniform buffer数据
        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, -2.0f, 2.0f),
                               glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.projection = glm::perspective(glm::radians(45.0f),
                                          (float)windowSize.width / (float)windowSize.height,
                                          0.1f, 10.0f);
        ubo.projection[1][1] *= -1;  // 适应Vulkan坐标系

        uniformBuffer.TransferData(&ubo, sizeof(ubo));

        graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
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
        fence.WaitAndReset();
        glfwPollEvents();
        TitleFps();


    }
    TerminateWindow();
    return 0;
}