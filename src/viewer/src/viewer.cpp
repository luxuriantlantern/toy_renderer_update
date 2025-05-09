//
// Created by clx on 25-3-20.
//

#include <iostream>
#include <fstream>
#include "viewer/viewer.h"

#include "EasyVulkan/GlfwGeneral.hpp"

void Viewer::initWindow(const std::string& title) {
    if(mRender->getType() == SHADER_BACKEND_TYPE::VULKAN)
    {
        return;
    }
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    mWindow = glfwCreateWindow(mwidth, mheight, title.c_str(), nullptr, nullptr);
    if (mWindow == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(mWindow);
    glfwSwapInterval(0);
    glfwSetFramebufferSizeCallback(mWindow, [](GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    });

    // Load GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return;
    }

    // Set up OpenGL state
    glEnable(GL_DEPTH_TEST);
}

void Viewer::initBackend() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    if (mRender->getType() == SHADER_BACKEND_TYPE::OPENGL) {
        ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
        ImGui_ImplOpenGL3_Init("#version 450");
    } else if (mRender->getType() == SHADER_BACKEND_TYPE::VULKAN) {
        ImGui_ImplGlfw_InitForVulkan(mWindow, true);
    }
}

void Viewer::mainloop()
{
    while (!glfwWindowShouldClose(mWindow)) {
        processInput(mWindow);
        glfwPollEvents();

        glfwGetWindowSize(mWindow, &mwidth, &mheight);

        if (mRender->getType() == SHADER_BACKEND_TYPE::OPENGL) {
            ImGui_ImplOpenGL3_NewFrame();
        } else if (mRender->getType() == SHADER_BACKEND_TYPE::VULKAN) {
            ImGui_ImplVulkan_NewFrame();
        }
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        mCamera->update(mwidth, mheight);
        if (mRender) {
            if(mRender->getType() != mShaderBackendType)
            {
//                TODO: switch to Vulkan
            }
            else{
                mRender->render(
                        mScene,
                        mCamera->getViewMatrix(),
                        mCamera->getProjectionMatrix()
                );
            }
        }
        for(const auto& ui : mUI)
        {
            ui->render();
        }

//        End loop
        if(mShaderBackendType == SHADER_BACKEND_TYPE::OPENGL)
        {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        else
        {
            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), mRender->getCurrentShader()->getCommandBuffer(), mRender->getCurrentShader()->getPipeline());
        }

        glfwSwapBuffers(mWindow);

        static double time0 = glfwGetTime();
        static double time1;
        static double dt;
        static int dframe = -1;
        static std::stringstream info;
        time1 = glfwGetTime();
        dframe++;
        if ((dt = time1 - time0) >= 1) {
            info.precision(1);
            info << "toy renderer" << "    " << std::fixed << dframe / dt << " FPS";
            glfwSetWindowTitle(mWindow, info.str().c_str());
            info.str("");
            time0 = time1;
            dframe = 0;
        }
    }
}

Viewer::~Viewer() {
    glfwTerminate();
}

void Viewer::processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // WASD movement (front, left, back, right) and space shift movement (up, down)
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        mCamera->moveUp(deltaTime, mMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        mCamera->moveDown(deltaTime, mMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        mCamera->moveLeft(deltaTime, mMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        mCamera->moveRight(deltaTime, mMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        mCamera->moveForward(deltaTime, mMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        mCamera->moveBackward(deltaTime, mMovementSpeed);
    }

    // Mouse movement

    int rightMouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
//    int leftMouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

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

        mCamera->processRightMouseMovement(xoffset, yoffset, mMouseSensitivity);
    }
    else if (rightMousePressed) {
        rightMousePressed = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    static auto scrollCallback = [](GLFWwindow* window, double xoffset, double yoffset) {
        auto viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));
        auto camera = viewer->getCamera();

        if (camera->getType() == PERSPECTIVE) {
            auto perspCamera = std::static_pointer_cast<PerspectiveCamera>(camera);
            float fov = perspCamera->getFov();

            fov -= static_cast<float>(yoffset) * 1.0f;

            fov = glm::clamp(fov, 5.0f, 180.0f);

            perspCamera->setFov(fov);
        }
    };

    static bool scrollCallbackSet = false;
    if (!scrollCallbackSet) {
        glfwSetWindowUserPointer(window, this);
        glfwSetScrollCallback(window, scrollCallback);
        scrollCallbackSet = true;
    }

    // if (leftMouseState == GLFW_PRESS) {
    //     if (!leftMousePressed) {
    //         leftMousePressed = true;
    //         firstMouse = true;
    //         glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //     }
    //
    //     if (firstMouse) {
    //         lastX = xpos;
    //         lastY = ypos;
    //         firstMouse = false;
    //     }
    //
    //     float xoffset = xpos - lastX;
    //     float yoffset = lastY - ypos;
    //
    //     lastX = xpos;
    //     lastY = ypos;
    //
    //     mCamera->processLeftMouseMovement(xoffset, yoffset, mMouseSensitivity, mMovementSpeed);
    // }
    // else if (leftMousePressed) {
    //     leftMousePressed = false;
    //     glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    // }
}