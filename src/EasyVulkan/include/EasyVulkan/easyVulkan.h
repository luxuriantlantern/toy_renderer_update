//
// Created by ftc on 25-4-18.
//

#ifndef LEARNVULKAN_EASYVULKAN_H
#define LEARNVULKAN_EASYVULKAN_H

#include "VKBase+.h"
using namespace vulkan;
extern VkExtent2D windowSize;

namespace easyVulkan {
    struct renderPassWithFramebuffers {
        vulkan::renderPass pass;
        std::vector<framebuffer> framebuffers;
    };
    renderPassWithFramebuffers& CreateRpwf_Screen();
    inline std::vector<depthStencilAttachment> dsas_screenWithDS;
    renderPassWithFramebuffers& CreateRpwf_ScreenWithDS(VkFormat depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT);
}

#endif //LEARNVULKAN_EASYVULKAN_H
