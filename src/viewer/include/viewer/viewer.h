//
// Created by clx on 25-3-20.
//

#ifndef VIEWER_H
#define VIEWER_H

#include "camera/orthographic.h"
#include "camera/perspective.h"
#include "scene/scene.h"
#include <GLFW/glfw3.h>
#include "render/render.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_vulkan.h>
#include <memory>

#include "ui/ui.h"

#include "EasyVulkan/GlfwGeneral.hpp"

class UI;

class Viewer {
public:
    Viewer() = default;
    Viewer(int width, int height, std::shared_ptr<Render>render_OpenGL, std::shared_ptr<Render> render_Vulkan,
            std::shared_ptr<Camera> camera, std::shared_ptr<Scene> scene, const std::string& title) {

        mWindow = nullptr;
        mCamera = std::move(camera);
        mScene = std::move(scene);
        mwidth = width;
        mheight = height;
        mRender_OpenGL = std::move(render_OpenGL);
        mRender_Vulkan = std::move(render_Vulkan);
        if(mShaderBackendType == SHADER_BACKEND_TYPE::OPENGL)
        {
            mCurrentRender = mRender_OpenGL;
            initWindow(title);
        }
        else
        {
            mCurrentRender = mRender_Vulkan;
            if (!InitializeWindow({static_cast<uint32_t>(width), static_cast<uint32_t>(height)}))
            {
                std::cerr << "Failed to initialize window" << std::endl;
                return;
            }
            windowSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;
            mWindow = pWindow;
            prevWindowSize = windowSize;
        }
        mCurrentRender->init();
        mCurrentRender->setup(mScene);
        initBackend();
        if(mShaderBackendType == SHADER_BACKEND_TYPE::VULKAN)
            initBackendVulkanImGUI();
    };
    ~Viewer();

    void initWindow(const std::string& title);
    void setWindow(GLFWwindow* window) { mWindow = window; }
    void initBackend();
    void initBackendVulkanImGUI();
    void mainloop();
    void switchBackend();
    void processInput(GLFWwindow* window);
    void addUI(const std::shared_ptr<UI>& ui) {
        mUI.push_back(ui);
    }
    void setCamera(const std::shared_ptr<Camera>& camera) { mCamera = camera; mCamera->update(mwidth, mheight); }
    std::shared_ptr<Camera> getCamera() { return mCamera; }
    std::shared_ptr<Scene> getScene() { return mScene; }
    std::shared_ptr<Render> getRender() {return mCurrentRender;}
    int getWidth() const { return mwidth; }
    int getHeight() const { return mheight; }
    float getMovementSpeed() const { return mMovementSpeed; }
    float getMouseSensitivity() const { return mMouseSensitivity; }
    void setMovementSpeed(float speed) { mMovementSpeed = speed; }
    void setMouseSensitivity(float sensitivity) { mMouseSensitivity = sensitivity; }
    SHADER_BACKEND_TYPE getBackendType() const { return mShaderBackendType; }
    void setSwitchType() { shouldswitch = true; }
    void cleanupVulkan();
    void cleanupOpenGL()
    {
        mCurrentRender->cleanup();
        for (auto& shader : mCurrentRender->getShaders()) {
            shader.second->cleanup();
        }
    };

private:
    GLFWwindow* mWindow;
    std::shared_ptr<Scene> mScene;
    std::shared_ptr<Camera> mCamera;
    float lastFrame = 0.0f;
    bool firstMouse = true;
    float lastX = 0.0f, lastY = 0.0f;
    bool rightMousePressed = false;
    bool leftMousePressed = false;
    float mMovementSpeed = 15.0f;
    float mMouseSensitivity = 0.1f;
    int mwidth, mheight;
    std::shared_ptr<Render> mRender_OpenGL, mRender_Vulkan, mCurrentRender;
    std::vector<std::shared_ptr<UI>> mUI;
    SHADER_BACKEND_TYPE mShaderBackendType = SHADER_BACKEND_TYPE::VULKAN;
    VkDescriptorPool mImGuiDescriptorPool = VK_NULL_HANDLE;
    VkExtent2D prevWindowSize;
    bool shouldswitch = false;
};



#endif //VIEWER_H
