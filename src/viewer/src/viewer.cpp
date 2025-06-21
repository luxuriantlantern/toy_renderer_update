//
// Created by clx on 25-3-20.
//

#include <iostream>
#include <fstream>
#include "camera/camera.h"
#include "viewer/viewer.h"

void Viewer::initWindow(const std::string& title) {
    glfwDefaultWindowHints();
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

    if (mCurrentRender->getType() == SHADER_BACKEND_TYPE::OPENGL) {
        ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
        ImGui_ImplOpenGL3_Init("#version 450");
    } else if (mCurrentRender->getType() == SHADER_BACKEND_TYPE::VULKAN) {
        VkDescriptorPoolSize pool_sizes[] =
                {
                        { VK_DESCRIPTOR_TYPE_SAMPLER, 100 },
                        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
                        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
                        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 }
                };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 100;
        pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
        pool_info.pPoolSizes = pool_sizes;

        VkResult err = vkCreateDescriptorPool(graphicsBase::Base().Device(), &pool_info, nullptr, &mImGuiDescriptorPool);
        if (err != VK_SUCCESS) {
            std::cerr << "无法创建 ImGui 描述符池: " << err << std::endl;
            return;
        }

    }
}

void Viewer::initBackendVulkanImGUI()
{
    ImGui_ImplGlfw_InitForVulkan(mWindow, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = graphicsBase::Base().Instance();
    init_info.PhysicalDevice = graphicsBase::Base().PhysicalDevice();
    init_info.Device = graphicsBase::Base().Device();
    init_info.QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(graphicsBase::Base().PhysicalDevice());
    init_info.Queue = graphicsBase::Base().getGraphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = mImGuiDescriptorPool;
    init_info.RenderPass = mCurrentRender->getCurrentShader()->RenderPassAndFramebuffers().pass;
    init_info.Subpass = 0;
    init_info.MinImageCount = graphicsBase::Base().SwapchainCreateInfo().minImageCount;
    init_info.ImageCount = graphicsBase::Base().SwapchainImageCount();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    auto check_vk_result = [](VkResult err) {
        if (err != 0)
            std::cerr << "Vulkan 错误: " << err << std::endl;
    };
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplVulkan_CreateFontsTexture();
    ImGui_ImplVulkan_DestroyFontsTexture();
}


void Viewer::mainloop()
{
    while (!glfwWindowShouldClose(mWindow)) {
         while (glfwGetWindowAttrib(mWindow, GLFW_ICONIFIED))
             glfwWaitEvents();

        if (shouldswitch) {
            switchBackend();
            shouldswitch = false;
            continue;
        }

         int width = 0, height = 0;
         // TODO: Avoid broken after changing window size
         glfwGetFramebufferSize(mWindow, &width, &height);
         if(mShaderBackendType == SHADER_BACKEND_TYPE::VULKAN) {
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
         }
        processInput(mWindow);

        glfwPollEvents();
        glfwGetWindowSize(mWindow, &mwidth, &mheight);

        if (mCurrentRender->getType() == SHADER_BACKEND_TYPE::OPENGL) {
            ImGui_ImplOpenGL3_NewFrame();
        } else if (mCurrentRender->getType() == SHADER_BACKEND_TYPE::VULKAN) {
            ImGui_ImplVulkan_NewFrame();
        }
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        for(auto& ui : mUI)
        {
            ui->render();
        }

        mCamera->update(mwidth, mheight);
        glm::mat4 proj = mCamera->getProjectionMatrix();
        if(mCurrentRender->getType() == SHADER_BACKEND_TYPE::VULKAN)
        {
            if(mCamera->getType() == CameraType::PERSPECTIVE)
            {
                for(int j = 0; j < 4; ++j)proj[j][1] *= -1;
            }
            else
            {
                proj[1][1] *= -1;
                proj[2][2] = proj[2][2] * 0.5f;
                proj[3][2] = (proj[3][2] + 1.0f) * 0.5f;
            }
        }
        mCurrentRender->render(mScene, mCamera->getViewMatrix(), proj);
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();

        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
        if (is_minimized)
            continue;

        if (mCurrentRender->getType() == SHADER_BACKEND_TYPE::OPENGL) {
            ImGui_ImplOpenGL3_RenderDrawData(draw_data);
        }
        else {
            auto &CommandBuffer = mCurrentRender->getCurrentShader()->getCommandBuffer().value();
            auto &rpwf = mCurrentRender->getCurrentShader()->RenderPassAndFramebuffers();
            ImGui_ImplVulkan_RenderDrawData(draw_data, CommandBuffer);
            rpwf.pass.CmdEnd(CommandBuffer);
            CommandBuffer.End();
            auto shader = mCurrentRender->getCurrentShader();
            fence &Fence = shader->getFence();
            semaphore &semaphore_imageIsAvailable = shader->getSemaphoreImageIsAvailable();
            semaphore &semaphore_renderingIsOver = shader->getSemaphoreRenderingIsOver();
            graphicsBase::Base().SubmitCommandBuffer_Graphics(CommandBuffer, semaphore_imageIsAvailable,
                                                              semaphore_renderingIsOver, Fence);
            graphicsBase::Base().PresentImage(semaphore_renderingIsOver);
            glfwPollEvents();

            Fence.WaitAndReset();
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

void Viewer::switchBackend()
{
    if (mCurrentRender->getType() == SHADER_BACKEND_TYPE::OPENGL) {
        ImGui_ImplOpenGL3_Shutdown();
        cleanupOpenGL();
    } else {
        ImGui_ImplVulkan_Shutdown();
        cleanupVulkan();
    }

    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (mWindow) {
        glfwDestroyWindow(mWindow);
        mWindow = nullptr;
    }

    SHADER_BACKEND_TYPE newBackendType = (mShaderBackendType == SHADER_BACKEND_TYPE::OPENGL)
                                         ? SHADER_BACKEND_TYPE::VULKAN
                                         : SHADER_BACKEND_TYPE::OPENGL;
    mShaderBackendType = newBackendType;
    mCurrentRender = (mShaderBackendType == SHADER_BACKEND_TYPE::OPENGL)
                     ? mRender_OpenGL
                     : mRender_Vulkan;

    if (mShaderBackendType == SHADER_BACKEND_TYPE::OPENGL) {
        initWindow("Toy Render");
        if (!mWindow) {
            std::cerr << "无法创建 OpenGL 窗口，后端切换失败" << std::endl;
            return;
        }
    } else {
        if (!InitializeWindow({static_cast<uint32_t>(mwidth), static_cast<uint32_t>(mheight)})) {
            std::cerr << "无法创建 Vulkan 窗口，后端切换失败" << std::endl;
            return;
        }
        windowSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;
        mWindow = pWindow;
        prevWindowSize = windowSize;
    }

    initBackend();


    if (mCurrentRender) {
        mCurrentRender->init();
        mCurrentRender->setup(mScene);
    }

    if (mShaderBackendType == SHADER_BACKEND_TYPE::VULKAN)initBackendVulkanImGUI();

    firstMouse = true;
    rightMousePressed = false;
    leftMousePressed = false;
}

void Viewer::cleanupVulkan() {
    if (mImGuiDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(graphicsBase::Base().Device(), mImGuiDescriptorPool, nullptr);
        mImGuiDescriptorPool = VK_NULL_HANDLE;
    }
    if(mCurrentRender)
    {
        graphicsBase::Base().WaitIdle();
        for(auto& shaders : mCurrentRender->getShaders()) {
            auto shader = shaders.second;
            shader->cleanup();
        }
//        auto& rpwf = mCurrentRender->getCurrentShader()->RenderPassAndFramebuffers();
//        rpwf.pass.~renderPass();
//        for(auto& framebuffer : rpwf.framebuffers) {
//            framebuffer.~framebuffer();
//        }
        mCurrentRender->cleanup();
        graphicsBase::Base().cleanup();
    }

}

Viewer::~Viewer() {
    if(mCurrentRender->getType() == SHADER_BACKEND_TYPE::VULKAN)cleanupVulkan();
    else cleanupOpenGL();
    if (mCurrentRender && mCurrentRender->getType() == SHADER_BACKEND_TYPE::VULKAN) {
        ImGui_ImplVulkan_Shutdown();
        if (mImGuiDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(graphicsBase::Base().Device(), mImGuiDescriptorPool, nullptr);
            mImGuiDescriptorPool = VK_NULL_HANDLE;
        }
    }
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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
}