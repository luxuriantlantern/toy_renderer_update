//
// Created by ftc on 25-6-21.
//

#ifndef TOY_RENDERER_UPDATE_UTIL_H
#define TOY_RENDERER_UPDATE_UTIL_H

#define DestroyHandleBy(Func) if (handle) { Func(graphicsBase::Base().Device(), handle, nullptr); handle = VK_NULL_HANDLE; }
#define MoveHandle handle = other.handle; other.handle = VK_NULL_HANDLE;
#define DefineHandleTypeOperator operator decltype(handle)() const { return handle; }
#define DefineAddressFunction const decltype(handle)* Address() const { return &handle; }

namespace vulkan{
    inline auto& outStream = std::cout;
}


#endif //TOY_RENDERER_UPDATE_UTIL_H
