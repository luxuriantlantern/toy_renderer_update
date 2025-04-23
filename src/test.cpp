#include "EasyVulkan/GlfwGeneral.hpp"
#include "EasyVulkan/easyVulkan.h"
#include <glslang/glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/StandAlone/DirStackFileIncluder.h>
#include <glslang/glslang/Public/ResourceLimits.h>
// #include <shaderc/shaderc.hpp>
using namespace vulkan;

descriptorSetLayout descriptorSetLayout_triangle;
pipelineLayout pipelineLayout_triangle;
pipeline pipeline_triangle;

const auto& RenderPassAndFramebuffers() {
    static const auto& rpwf = easyVulkan::CreateRpwf_Screen();
    return rpwf;
}
void CreateLayout() {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_trianglePosition = {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    };
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo_triangle = {
            .bindingCount = 1,
            .pBindings = &descriptorSetLayoutBinding_trianglePosition
    };
    descriptorSetLayout_triangle.Create(descriptorSetLayoutCreateInfo_triangle);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
            .setLayoutCount = 1,
            .pSetLayouts = descriptorSetLayout_triangle.Address()
    };
    pipelineLayout_triangle.Create(pipelineLayoutCreateInfo);
}

void InitGlslang() {
    glslang::InitializeProcess();
}

// 关闭 glslang
void FinalizeGlslang() {
    glslang::FinalizeProcess();
}


// 编译 GLSL 到 SPIR-V（返回二进制数据）
std::vector<uint32_t> CompileGLSLToSPIRV(const std::string& shaderSource, EShLanguage stage) {
    glslang::TShader shader(stage);
    const char* source = shaderSource.c_str();
    shader.setStrings(&source, 1);

    // 设置 Vulkan 环境
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    // 编译选项
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(GetDefaultResources(), 100, false, messages)) {
        std::cerr << "Shader compilation failed:\n" << shader.getInfoLog() << std::endl;
        return {};
    }

    // 链接程序
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        std::cerr << "Shader linking failed:\n" << program.getInfoLog() << std::endl;
        return {};
    }

    // 生成 SPIR-V
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
    glm::vec3 position;  // 改为vec3
    glm::vec4 color;
};

void CreatePipeline() {
    InitGlslang();

    // 读取修改后的着色器文件
    std::string vertShaderCode = readFile("./assets/shaders/cube.vert");
    std::string fragShaderCode = readFile("./assets/shaders/cube.frag");

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

        // 修改顶点输入绑定和属性
        pipelineCiPack.vertexInputBindings.emplace_back(0, sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX);
        pipelineCiPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, position));
        pipelineCiPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex, color));

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
    windowSize  = graphicsBase::Base().SwapchainCreateInfo().imageExtent;
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


    vertex vertices[] = {
            // Front face
            {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},

            // Back face
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},

            // Left face
            {{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},

            // Right face
            {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},

            // Top face
            {{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},

            // Bottom face
            {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}}
    };

    vertexBuffer vertexBuffer(sizeof(vertices));
    vertexBuffer.TransferData(vertices);

    // 修改uniform buffer为存储MVP矩阵
    uniformBuffer uniformBuffer(sizeof(glm::mat4));

    VkDescriptorPoolSize descriptorPoolSizes[] = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
    };
    descriptorPool descriptorPool(1, descriptorPoolSizes);
    descriptorSet descriptorSet_trianglePosition;
    descriptorPool.AllocateSets(descriptorSet_trianglePosition, descriptorSetLayout_triangle);
    VkDescriptorBufferInfo bufferInfo = {
            .buffer = uniformBuffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE
    };
    descriptorSet_trianglePosition.Write(bufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    VkClearValue clearColor = { .color = { 1.f, 0.f, 0.f, 1.f } };

    while (!glfwWindowShouldClose(pWindow)) {
        while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED))
            glfwWaitEvents();

        int width = 0, height = 0;
        glfwGetFramebufferSize(pWindow, &width, &height);

        // Check for window resize
        if (width > 0 && height > 0 &&
            (width != prevWindowSize.width || height != prevWindowSize.height)) {

            // Wait for the device to finish operations before recreating swapchain
            graphicsBase::Base().WaitIdle();

            // Update window size
            windowSize.width = width;
            windowSize.height = height;

            // Recreate swapchain
            if (graphicsBase::Base().RecreateSwapchain()) {
                std::cerr << "Failed to recreate swapchain\n";
                break;
            }

            // Update the previous size
            prevWindowSize = windowSize;
            continue; // Skip this frame and start with the new swapchain
        }
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float>(currentTime - startTime).count();

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
                                     glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                          (float)windowSize.width / (float)windowSize.height,
                                          0.1f, 10.0f);
        proj[1][1] *= -1;  // 适应Vulkan坐标系

        glm::mat4 mvp = proj * view * model;
        uniformBuffer.TransferData(&mvp, sizeof(mvp));

        graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
        auto i = graphicsBase::Base().CurrentImageIndex();

        commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        renderPass.CmdBegin(commandBuffer, framebuffers[i], { {}, windowSize }, clearColor);
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.Address(), &offset);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_triangle);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout_triangle, 0, 1, descriptorSet_trianglePosition.Address(), 0, nullptr);
        vkCmdDraw(commandBuffer, 36, 1, 0, 0);
        renderPass.CmdEnd(commandBuffer);
        commandBuffer.End();

        graphicsBase::Base().SubmitCommandBuffer_Graphics(commandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);
        graphicsBase::Base().PresentImage(semaphore_renderingIsOver);

        glfwPollEvents();
        TitleFps();

        fence.WaitAndReset();
    }
    TerminateWindow();
    return 0;
}