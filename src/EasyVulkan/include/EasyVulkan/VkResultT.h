//
// Created by ftc on 25-6-21.
//

#ifndef TOY_RENDERER_UPDATE_VKRESULTT_H
#define TOY_RENDERER_UPDATE_VKRESULTT_H

#include <vulkan/vulkan.h>

namespace vulkan{
    class result_t {
        VkResult result;
    public:
        static void(*callback_throw)(VkResult);
        result_t(VkResult result) :result(result) {}
        result_t(result_t&& other) noexcept :result(other.result) { other.result = VK_SUCCESS; }
        ~result_t() noexcept(false) {
            if (uint32_t(result) < VK_RESULT_MAX_ENUM)
                return;
            if (callback_throw)
                callback_throw(result);
            throw result;
        }
        operator VkResult() {
            VkResult result = this->result;
            this->result = VK_SUCCESS;
            return result;
        }
    };
    inline void(*result_t::callback_throw)(VkResult);
}

#endif //TOY_RENDERER_UPDATE_VKRESULTT_H
