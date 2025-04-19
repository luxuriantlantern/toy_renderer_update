#include "EasyVulkan/GlfwGeneral.hpp"
#include "EasyVulkan/easyVulkan.h"
#include <glslang/glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/StandAlone/DirStackFileIncluder.h>
#include <glslang/glslang/Public/ResourceLimits.h>
#include <shaderc/shaderc.hpp>
using namespace vulkan;

pipelineLayout pipelineLayout_triangle;
pipeline pipeline_triangle;
const auto& RenderPassAndFramebuffers() {
    static const auto& rpwf = easyVulkan::CreateRpwf_Screen();
    return rpwf;
}
void CreateLayout() {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
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

void CreatePipeline() {

    InitGlslang();
    std::string vertShaderCode = readFile("./assets/shaders/FirstTriangle.vert");
    std::string fragShaderCode = readFile("./assets/shaders/FirstTriangle.frag");
    std::vector<uint32_t> vertSpirv = CompileGLSLToSPIRV(vertShaderCode, EShLangVertex);
    std::vector<uint32_t> fragSpirv = CompileGLSLToSPIRV(fragShaderCode, EShLangFragment);

//    static shaderModule vert("./assets/shaders/FirstTriangle.vert.spv");
//    static shaderModule frag("./assets/shaders/FirstTriangle.frag.spv");

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
        pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        float width = std::max(1.0f, float(windowSize.width));
        float height = std::max(1.0f, float(windowSize.height));

        pipelineCiPack.viewports.emplace_back(0.f, 0.f, width, height, 0.f, 1.f);
        pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
        pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
        pipelineCiPack.UpdateAllArrays();
        pipelineCiPack.createInfo.stageCount = 2;
        pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_triangle;
        pipeline_triangle.Create(pipelineCiPack);
    };
    auto Destroy = [] {
        pipeline_triangle.~pipeline();
    };
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

        graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
        auto i = graphicsBase::Base().CurrentImageIndex();

        commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        renderPass.CmdBegin(commandBuffer, framebuffers[i], { {}, windowSize }, clearColor);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_triangle);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
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