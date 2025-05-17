#ifndef TOY_RENDERER_UPDATE_GLFWGENERAL_H
#define TOY_RENDERER_UPDATE_GLFWGENERAL_H

#include "VKBase+.h"
#define GLFW_INCLUDE_VULKAN
#include "glfw/include/GLFW/glfw3.h"
//#pragma comment(lib, "glfw3.lib")

#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

extern GLFWwindow* pWindow;
extern GLFWmonitor* pMonitor;
extern const char* windowTitle;

bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = false, bool limitFrameRat = false);
void TerminateWindow();
void MakeWindowFullScreen();
void MakeWindowWindowed(VkOffset2D position, VkExtent2D size);
void TitleFps();



#endif