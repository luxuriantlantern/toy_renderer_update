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
#include "render/render_OpenGL.h"
#include "scene/scene.h"
#include "camera/perspective.h"
#include <memory>
#include <GLFW/glfw3.h>
#include <iostream>
#include <viewer/viewer.h>
#include <viewer/ui/ui.h>
#include <viewer/ui/cameraUI.h>
#include <viewer/ui/modelUI.h>
#include <viewer/ui/shaderUI.h>


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


    auto render = std::make_shared<Render_Vulkan>();
    auto render2 = std::make_shared<Render_OpenGL>();

    auto viewer = std::make_shared<Viewer>(WIDTH, HEIGHT, render2, render, gCamera, scene, title);
    const auto ui_model = std::make_shared<ModelUI>(viewer);
    const auto ui_shader = std::make_shared<ShaderUI>(viewer);
    const auto ui_camera = std::make_shared<CameraUI>(viewer);
    viewer->addUI(ui_model);
    viewer->addUI(ui_shader);
    viewer->addUI(ui_camera);
    viewer->mainloop();
}