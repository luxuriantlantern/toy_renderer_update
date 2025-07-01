//
// Created by ftc on 25-6-21.
//

#ifndef TOY_RENDERER_UPDATE_VKGRAPHICSBASEPLUS_H
#define TOY_RENDERER_UPDATE_VKGRAPHICSBASEPLUS_H

#include "VKBase.h"
#include "VKFormat.h"
#include "VkCommandPool.h"
#include "VkCommandBuffer.h"
namespace vulkan
{
    class graphicsBasePlus : public graphicsBase {
    public:
        VkFormatProperties formatProperties[std::size(formatInfos_v1_0)] = {};
        commandPool* commandPool_graphics = nullptr;
        commandPool* commandPool_presentation = nullptr;
        commandPool* commandPool_compute = nullptr;
        commandBuffer* commandBuffer_transfer = nullptr;
        commandBuffer* commandBuffer_presentation = nullptr;
        //Static
//		static graphicsBasePlus singleton;
    public://--------------------



        graphicsBasePlus() = default;

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
        result_t ExecuteCommandBuffer_Graphics(graphicsBase *gb, VkCommandBuffer commandBuffer) const {
            fence fence;
            VkSubmitInfo submitInfo = {
                    .commandBufferCount = 1,
                    .pCommandBuffers = &commandBuffer
            };
            VkResult result = gb->SubmitCommandBuffer_Graphics(submitInfo, fence);
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
