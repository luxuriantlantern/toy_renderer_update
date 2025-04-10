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
        initWindow(title);
    };
    ~Viewer();

    void initWindow(const std::string& title);
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
};



#endif //VIEWER_H
