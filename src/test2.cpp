//
// Created by ftc on 25-4-25.
//

#include "scene/scene.h"
#include "camera/perspective.h"
#include <memory>
#include "EasyVulkan/GlfwGeneral.hpp"
#include "EasyVulkan/easyVulkan.h"
#include <glslang/glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/StandAlone/DirStackFileIncluder.h>
#include <glslang/glslang/Public/ResourceLimits.h>
#include "render/render_Vulkan.h"


int main()
{
    const int WIDTH = 1920, HEIGHT = 1080;
    const std::string title = "toy renderer";
    auto scene = std::make_shared<Scene>();
    scene->addModel("./assets/SJTU_east_gate_MC/East_Gate_Voxel.obj");
//    scene->addModel("./assets/cube2.ply");
    auto camera = std::make_shared<PerspectiveCamera>();

    camera->update(WIDTH, HEIGHT);

    if (!InitializeWindow({WIDTH, HEIGHT}))
        return -1;
    windowSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;
    VkExtent2D prevWindowSize = windowSize;

    auto render = std::make_shared<Render_Vulkan>();
    render->init();
    render->setup(scene);


    while(!glfwWindowShouldClose(pWindow))
    {
        glfwPollEvents();
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
        render->render(scene, camera->getViewMatrix(), camera->getProjectionMatrix());
        glfwSwapBuffers(pWindow);
    }
}