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
// test2.cpp
#include "scene/scene.h"
#include "camera/perspective.h"
#include <memory>
#include <GLFW/glfw3.h>
#include <iostream>

// 全局变量
std::shared_ptr<PerspectiveCamera> gCamera;
float gMovementSpeed = 15.0f;
float gMouseSensitivity = 0.1f;
float lastFrame = 0.0f;
bool rightMousePressed = false;
bool firstMouse = true;
double lastX = 0.0, lastY = 0.0;

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        gCamera->moveUp(deltaTime, gMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        gCamera->moveDown(deltaTime, gMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        gCamera->moveLeft(deltaTime, gMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        gCamera->moveRight(deltaTime, gMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        gCamera->moveForward(deltaTime, gMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        gCamera->moveBackward(deltaTime, gMovementSpeed);
    }

    int rightMouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (rightMouseState == GLFW_PRESS) {
        if (!rightMousePressed) {
            rightMousePressed = true;
            firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;
        gCamera->processRightMouseMovement(xoffset, yoffset, gMouseSensitivity);
    } else if (rightMousePressed) {
        rightMousePressed = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

int main() {
    const int WIDTH = 1920, HEIGHT = 1080;
    const std::string title = "toy renderer";
    auto scene = std::make_shared<Scene>();
    scene->addModel("./assets/SJTU_east_gate_MC/East_Gate_Voxel.obj");
    gCamera = std::make_shared<PerspectiveCamera>();
    gCamera->update(WIDTH, HEIGHT);

    if (!InitializeWindow({WIDTH, HEIGHT}))
        return -1;
    windowSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;
    VkExtent2D prevWindowSize = windowSize;

    auto render = std::make_shared<Render_Vulkan>();
    render->init();
    render->setup(scene);
    float basetime = glfwGetTime();
    while(!glfwWindowShouldClose(pWindow)) {
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

        processInput(pWindow); // 调用输入处理
        gCamera->update(width, height);
        render->render(scene, gCamera->getViewMatrix(), gCamera->getProjectionMatrix());
        if(glfwGetTime() - basetime > 5.0f)
        {
            basetime = glfwGetTime();
            if(render->getShaderType() == SHADER_TYPE::Blinn_Phong)
                render->setShaderType(SHADER_TYPE::MATERIAL);
            else
                render->setShaderType(SHADER_TYPE::Blinn_Phong);
        }
        TitleFps();
    }
    glfwTerminate();
}