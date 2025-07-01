//
// Created by ftc on 25-6-21.
//

#ifndef TOY_RENDERER_UPDATE_VKCOMMANDBUFFER_H
#define TOY_RENDERER_UPDATE_VKCOMMANDBUFFER_H

#include <vulkan/vulkan.h>
#include <format>
#include "VkResultT.h"
#include "VkUtil.h"

namespace vulkan {
    class commandBuffer {
        friend class commandPool;
        VkCommandBuffer handle = VK_NULL_HANDLE;
    public:
        commandBuffer() = default;
        commandBuffer(commandBuffer&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
        //Getter
        operator decltype(handle)() const { return handle; }
        const decltype(handle)* Address() const { return &handle; }
        //Const Function
        result_t Begin(VkCommandBufferUsageFlags usageFlags, VkCommandBufferInheritanceInfo& inheritanceInfo) const {
            inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            VkCommandBufferBeginInfo beginInfo = {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                    .flags = usageFlags,
                    .pInheritanceInfo = &inheritanceInfo
            };
            VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
            if (result)
                outStream << std::format("[ commandBuffer ] ERROR\nFailed to begin a command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t Begin(VkCommandBufferUsageFlags usageFlags = 0) const {
            VkCommandBufferBeginInfo beginInfo = {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                    .flags = usageFlags,
            };
            VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
            if (result)
                outStream << std::format("[ commandBuffer ] ERROR\nFailed to begin a command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t End() const {
            VkResult result = vkEndCommandBuffer(handle);
            if (result)
                outStream << std::format("[ commandBuffer ] ERROR\nFailed to end a command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }

    };

}

#endif //TOY_RENDERER_UPDATE_VKCOMMANDBUFFER_H
