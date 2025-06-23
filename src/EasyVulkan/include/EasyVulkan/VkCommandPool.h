//
// Created by ftc on 25-6-21.
//

#ifndef TOY_RENDERER_UPDATE_VKCOMMANDPOOL_H
#define TOY_RENDERER_UPDATE_VKCOMMANDPOOL_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <sstream>
#include <ostream>
#include <format>
#include <memory>
#include "VkResultT.h"
#include "VkUtil.h"
#include "EasyVKStart.h"
#include "VkCommandBuffer.h"

namespace vulkan {
    class commandPool {
        VkCommandPool handle = VK_NULL_HANDLE;
        VkDevice * mDevice = nullptr;
    public:
        explicit commandPool(VkDevice *device) : mDevice(std::move(device)) {}
        commandPool(VkCommandPoolCreateInfo& createInfo) {
            Create(createInfo);
        }
        commandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
            Create(queueFamilyIndex, flags);
        }
        commandPool(commandPool&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
        ~commandPool() { if (handle) { vkDestroyCommandPool(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
        //Getter
        operator decltype(handle)() const { return handle; }
        const decltype(handle)* Address() const { return &handle; }
        //Const Function
        result_t AllocateBuffers(arrayRef<VkCommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
            VkCommandBufferAllocateInfo allocateInfo = {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .commandPool = handle,
                    .level = level,
                    .commandBufferCount = uint32_t(buffers.Count())
            };
            VkResult result = vkAllocateCommandBuffers(*mDevice, &allocateInfo, buffers.Pointer());
            if (result)
                outStream << std::format("[ commandPool ] ERROR\nFailed to allocate command buffers!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t AllocateBuffers(arrayRef<commandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
            return AllocateBuffers(
                    { &buffers[0].handle, buffers.Count() },
                    level);
        }
        void FreeBuffers(arrayRef<VkCommandBuffer> buffers) const {
            vkFreeCommandBuffers(*mDevice, handle, buffers.Count(), buffers.Pointer());
            memset(buffers.Pointer(), 0, buffers.Count() * sizeof(VkCommandBuffer));
        }
        void FreeBuffers(arrayRef<commandBuffer> buffers) const {
            FreeBuffers({ &buffers[0].handle, buffers.Count() });
        }
        //Non-const Function
        result_t Create(VkCommandPoolCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            VkResult result = vkCreateCommandPool(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ commandPool ] ERROR\nFailed to create a command pool!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t Create(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
            VkCommandPoolCreateInfo createInfo = {
                    .flags = flags,
                    .queueFamilyIndex = queueFamilyIndex
            };
            return Create(createInfo);
        }
    };

}

#endif //TOY_RENDERER_UPDATE_VKCOMMANDPOOL_H
