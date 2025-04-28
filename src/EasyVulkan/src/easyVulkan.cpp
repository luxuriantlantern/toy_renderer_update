//
// Created by ftc on 25-4-25.
//

#include "EasyVulkan/easyVulkan.h"

VkExtent2D windowSize = { 0, 0 };

namespace easyVulkan{
    renderPassWithFramebuffers& CreateRpwf_Screen() {
        static renderPassWithFramebuffers rpwf;

        VkAttachmentDescription attachmentDescription = {
                .format = graphicsBase::Base().SwapchainCreateInfo().imageFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        };
        VkAttachmentReference attachmentReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        VkSubpassDescription subpassDescription = {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 1,
                .pColorAttachments = &attachmentReference
        };
        VkSubpassDependency subpassDependency = {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                //You may use VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT if synchronization is done by semaphore.
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
        };
        VkRenderPassCreateInfo renderPassCreateInfo = {
                .attachmentCount = 1,
                .pAttachments = &attachmentDescription,
                .subpassCount = 1,
                .pSubpasses = &subpassDescription,
                .dependencyCount = 1,
                .pDependencies = &subpassDependency
        };
        rpwf.pass.Create(renderPassCreateInfo);

        auto CreateFramebuffers = [] {
            rpwf.framebuffers.resize(graphicsBase::Base().SwapchainImageCount());
            VkFramebufferCreateInfo framebufferCreateInfo = {
                    .renderPass = rpwf.pass,
                    .attachmentCount = 1,
                    .width = windowSize.width,
                    .height = windowSize.height,
                    .layers = 1
            };
            for (size_t i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++) {
                VkImageView attachment = graphicsBase::Base().SwapchainImageView(i);
                framebufferCreateInfo.pAttachments = &attachment;
                rpwf.framebuffers[i].Create(framebufferCreateInfo);
            }
        };
        auto DestroyFramebuffers = [] {
            rpwf.framebuffers.clear();
        };
        CreateFramebuffers();

        ExecuteOnce(rpwf);
        graphicsBase::Base().AddCallback_CreateSwapchain(CreateFramebuffers);
        graphicsBase::Base().AddCallback_DestroySwapchain(DestroyFramebuffers);
        return rpwf;
    }
    renderPassWithFramebuffers& CreateRpwf_ScreenWithDS(VkFormat depthStencilFormat) {
		static renderPassWithFramebuffers rpwf;
		static VkFormat _depthStencilFormat = depthStencilFormat;

		VkAttachmentDescription attachmentDescriptions[2] = {
			{//Color attachment
				.format = graphicsBase::Base().SwapchainCreateInfo().imageFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },
			{//Depth stencil attachment
				.format = _depthStencilFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = _depthStencilFormat != VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = _depthStencilFormat >= VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
		};
		VkAttachmentReference attachmentReferences[2] = {
			{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			//Unless the separateDepthStencilLayouts feature is enabled, even if depthStencilFormat doesn't support stencil, layout must be VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.
			{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
		};
		VkSubpassDescription subpassDescription = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = attachmentReferences,
			.pDepthStencilAttachment = attachmentReferences + 1
		};
		//At EARLY_FRAGMENT_TESTS stage, the ds image'll be cleared (if performs) then readed, ds tests are performed for each fragment.
		//At LATE_FRAGMENT_TESTS stage, ds tests are performed for each sample.
		VkSubpassDependency subpassDependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,//Because of VK_ATTACHMENT_LOAD_OP_CLEAR
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};
		VkRenderPassCreateInfo renderPassCreateInfo = {
			.attachmentCount = 2,
			.pAttachments = attachmentDescriptions,
			.subpassCount = 1,
			.pSubpasses = &subpassDescription,
			.dependencyCount = 1,
			.pDependencies = &subpassDependency
		};
		rpwf.pass.Create(renderPassCreateInfo);
		auto CreateFramebuffers = [] {
			dsas_screenWithDS.resize(graphicsBase::Base().SwapchainImageCount());
			rpwf.framebuffers.resize(graphicsBase::Base().SwapchainImageCount());
			for (auto& i : dsas_screenWithDS)
				i.Create(_depthStencilFormat, windowSize, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
			VkFramebufferCreateInfo framebufferCreateInfo = {
				.renderPass = rpwf.pass,
				.attachmentCount = 2,
				.width = windowSize.width,
				.height = windowSize.height,
				.layers = 1
			};
			for (size_t i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++) {
				VkImageView attachments[2] = {
					graphicsBase::Base().SwapchainImageView(i),
					dsas_screenWithDS[i].ImageView()
				};
				framebufferCreateInfo.pAttachments = attachments;
				rpwf.framebuffers[i].Create(framebufferCreateInfo);
			}
		};
		auto DestroyFramebuffers = [] {
			dsas_screenWithDS.clear();
			rpwf.framebuffers.clear();
		};
		CreateFramebuffers();

		ExecuteOnce(rpwf);
		graphicsBase::Base().AddCallback_CreateSwapchain(CreateFramebuffers);
		graphicsBase::Base().AddCallback_DestroySwapchain(DestroyFramebuffers);
		return rpwf;
	}
}