//
// Created by ftc on 25-4-18.
//

#ifndef LEARNVULKAN_EASYVULKAN_H
#define LEARNVULKAN_EASYVULKAN_H

#include "VKBase.h"
using namespace vulkan;
extern VkExtent2D windowSize;

namespace easyVulkan {
    struct renderPassWithFramebuffers {
        vulkan::renderPass pass;
        std::vector<framebuffer> framebuffers;
    };
    renderPassWithFramebuffers& CreateRpwf_Screen();
}

#endif //LEARNVULKAN_EASYVULKAN_H
