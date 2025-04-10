//
// Created by clx on 25-3-20.
//

#include <iostream>
#include <fstream>
#include "viewer.h"
#include "render/render_OpenGL.h"

void Viewer::initWindow(const std::string& title) {
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

void Viewer::mainloop()
{
    while (!glfwWindowShouldClose(mWindow)) {
        processInput(mWindow);
        glfwGetWindowSize(mWindow, &mwidth, &mheight);
        mCamera->update(mwidth, mheight);
        if (mRender) {
            mRender->render(
                mScene,
                mCamera->getViewMatrix(),
                mCamera->getProjectionMatrix()
            );
        }
        glfwSwapBuffers(mWindow);
        glfwPollEvents();
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

    // WASD movement (up, left, down, right) and JK movement (forward, backward)
    if (glfwGetKey(window, GLFW_KEY_W) || glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        mCamera->moveUp(deltaTime, mMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        mCamera->moveDown(deltaTime, mMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        mCamera->moveLeft(deltaTime, mMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        mCamera->moveRight(deltaTime, mMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
        mCamera->moveForward(deltaTime, mMovementSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        mCamera->moveBackward(deltaTime, mMovementSpeed);
    }

    // Mouse movement

    int rightMouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    int leftMouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

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

    if (leftMouseState == GLFW_PRESS) {
        if (!leftMousePressed) {
            leftMousePressed = true;
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

        mCamera->processLeftMouseMovement(xoffset, yoffset, mMouseSensitivity, mMovementSpeed);
    }
    else if (leftMousePressed) {
        leftMousePressed = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}