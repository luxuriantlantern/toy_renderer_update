//
// Created by ftc on 25-6-21.
//

#ifndef TOY_RENDERER_UPDATE_VKGRAPHICSBASEPLUS_H
#define TOY_RENDERER_UPDATE_VKGRAPHICSBASEPLUS_H

#include "VKBase.h"
#include "VKFormat.h"
namespace vulkan
{
    class graphicsBasePlus {
        friend class graphicsBase;
        VkFormatProperties formatProperties[std::size(formatInfos_v1_0)] = {};
        commandPool commandPool_graphics;
        commandPool commandPool_presentation;
        commandPool commandPool_compute;
        commandBuffer commandBuffer_transfer;
        commandBuffer commandBuffer_presentation;
        //Static
//		static graphicsBasePlus singleton;
    public://--------------------
        graphicsBasePlus() {
//			auto Initialize = [] {
//				if (graphicsBase::Base().QueueFamilyIndex_Graphics() != VK_QUEUE_FAMILY_IGNORED)
//					singleton.commandPool_graphics.Create(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
//					singleton.commandPool_graphics.AllocateBuffers(singleton.commandBuffer_transfer);
//				if (graphicsBase::Base().QueueFamilyIndex_Compute() != VK_QUEUE_FAMILY_IGNORED)
//					singleton.commandPool_compute.Create(graphicsBase::Base().QueueFamilyIndex_Compute(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
//				if (graphicsBase::Base().QueueFamilyIndex_Presentation() != VK_QUEUE_FAMILY_IGNORED &&
//					graphicsBase::Base().QueueFamilyIndex_Presentation() != graphicsBase::Base().QueueFamilyIndex_Graphics() &&
//					graphicsBase::Base().SwapchainCreateInfo().imageSharingMode == VK_SHARING_MODE_EXCLUSIVE)
//					singleton.commandPool_presentation.Create(graphicsBase::Base().QueueFamilyIndex_Presentation(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
//					singleton.commandPool_presentation.AllocateBuffers(singleton.commandBuffer_presentation);
//				for (size_t i = 0; i < std::size(singleton.formatProperties); i++)
//					vkGetPhysicalDeviceFormatProperties(graphicsBase::Base().PhysicalDevice(), VkFormat(i), &singleton.formatProperties[i]);
//			};
//			auto CleanUp = [] {
//				singleton.commandPool_graphics.~commandPool();
//				singleton.commandPool_presentation.~commandPool();
//				singleton.commandPool_compute.~commandPool();
//			};
//			graphicsBase::Plus(singleton);
//			graphicsBase::Base().AddCallback_CreateDevice(Initialize);
//			graphicsBase::Base().AddCallback_DestroyDevice(CleanUp);
        }

//
        graphicsBasePlus(graphicsBasePlus &&) = delete;

        ~graphicsBasePlus() = default;

//        void cleanup() {this->~graphicsBasePlus(); new(&singleton) graphicsBasePlus(); }
        //Getter
        const VkFormatProperties &FormatProperties(VkFormat format) const {
#ifndef NDEBUG
            if (uint32_t(format) >= std::size(formatInfos_v1_0))
                outStream << std::format(
                        "[ FormatProperties ] ERROR\nThis function only supports definite formats provided by VK_VERSION_1_0.\n"),
                        abort();
#endif
            return formatProperties[format];
        }

        const commandPool &CommandPool_Graphics() const { return commandPool_graphics; }

        const commandPool &CommandPool_Compute() const { return commandPool_compute; }

        const commandBuffer &CommandBuffer_Transfer() const { return commandBuffer_transfer; }

        //Const Function
        result_t ExecuteCommandBuffer_Graphics(VkCommandBuffer commandBuffer) const {
            fence fence;
            VkSubmitInfo submitInfo = {
                    .commandBufferCount = 1,
                    .pCommandBuffers = &commandBuffer
            };
            VkResult result = graphicsBase::Base().SubmitCommandBuffer_Graphics(submitInfo, fence);
            if (!result)
                fence.Wait();
            return result;
        }

        result_t ExecuteCommandBuffer_Compute(VkCommandBuffer commandBuffer) const {
            fence fence;
            VkSubmitInfo submitInfo = {
                    .commandBufferCount = 1,
                    .pCommandBuffers = &commandBuffer
            };
            VkResult result = graphicsBase::Base().SubmitCommandBuffer_Compute(submitInfo, fence);
            if (!result)
                fence.Wait();
            return result;
        }

        result_t AcquireImageOwnership_Presentation(VkSemaphore semaphore_renderingIsOver,
                                                    VkSemaphore semaphore_ownershipIsTransfered,
                                                    VkFence fence = VK_NULL_HANDLE) const {
            if (VkResult result = commandBuffer_presentation.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
                return result;
            graphicsBase::Base().CmdTransferImageOwnership(commandBuffer_presentation);
            if (VkResult result = commandBuffer_presentation.End())
                return result;
            return graphicsBase::Base().SubmitCommandBuffer_Presentation(commandBuffer_presentation,
                                                                         semaphore_renderingIsOver,
                                                                         semaphore_ownershipIsTransfered, fence);
        }
    };
}
#endif //TOY_RENDERER_UPDATE_VKGRAPHICSBASEPLUS_H
