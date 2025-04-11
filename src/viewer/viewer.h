//
// Created by clx on 25-3-20.
//

#ifndef VIEWER_H
#define VIEWER_H

#include "camera/orthographic.h"
#include "camera/perspective.h"
#include "scene/scene.h"
#include "GLFW/glfw3.h"
#include "render/render.h"
#include "viewer/ui/ui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <memory>

class Viewer {
public:
    Viewer() = default;
    Viewer(int width, int height, std::shared_ptr<Render>render, std::shared_ptr<Camera> camera, std::shared_ptr<Scene> scene, const std::string& title) {
        mWindow = nullptr;
        mCamera = std::move(camera);
        mScene = std::move(scene);
        mwidth = width;
        mheight = height;
        mRender = std::move(render);
        mUI = std::make_shared<UI>();
        initWindow(title);
        initBackend();
    };
    ~Viewer();

    void initWindow(const std::string& title);
    void initBackend();
    void mainloop();
    void processInput(GLFWwindow* window);

    std::shared_ptr<Scene> getScene() { return mScene; }

private:
    GLFWwindow* mWindow;
    std::shared_ptr<Scene> mScene;
    std::shared_ptr<Camera> mCamera;
    float lastFrame = 0.0f;
    bool firstMouse = true;
    float lastX = 0.0f, lastY = 0.0f;
    bool rightMousePressed = false;
    bool leftMousePressed = false;
    float mMovementSpeed = 2.5f;
    float mMouseSensitivity = 0.1f;
    int mwidth, mheight;
    std::shared_ptr<Render> mRender;
    std::shared_ptr<UI> mUI;
    SHADER_BACKEND_TYPE mShaderBackendType = SHADER_BACKEND_TYPE::OPENGL;
};



#endif //VIEWER_H
