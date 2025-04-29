#ifndef TOY_RENDERER_UPDATE_VKBASE_H
#define TOY_RENDERER_UPDATE_VKBASE_H

#pragma once
#include "EasyVKStart.h"
#include "VKFormat.h"
#define VK_RESULT_THROW

#define DestroyHandleBy(Func) if (handle) { Func(graphicsBase::Base().Device(), handle, nullptr); handle = VK_NULL_HANDLE; }
#define MoveHandle handle = other.handle; other.handle = VK_NULL_HANDLE;
#define DefineHandleTypeOperator operator decltype(handle)() const { return handle; }
#define DefineAddressFunction const decltype(handle)* Address() const { return &handle; }

#ifndef NDEBUG
#define ENABLE_DEBUG_MESSENGER true
#else
#define ENABLE_DEBUG_MESSENGER false
#endif

namespace vulkan {
    constexpr VkExtent2D defaultWindowSize = { 1920, 1080 };
    inline auto& outStream = std::cout;

#ifdef VK_RESULT_THROW
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

#elif defined VK_RESULT_NODISCARD
    struct [[nodiscard]] result_t {
		VkResult result;
		result_t(VkResult result) :result(result) {}
		operator VkResult() const { return result; }
	};
#pragma warning(disable:4834)
#pragma warning(disable:6031)
#else
	using result_t = VkResult;
#endif
    class graphicsBasePlus;

    class graphicsBase {
        uint32_t apiVersion = VK_API_VERSION_1_0;
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkPhysicalDeviceProperties physicalDeviceProperties;
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        std::vector<VkPhysicalDevice> availablePhysicalDevices;

        VkDevice device;
        uint32_t queueFamilyIndex_graphics = VK_QUEUE_FAMILY_IGNORED;
        uint32_t queueFamilyIndex_presentation = VK_QUEUE_FAMILY_IGNORED;
        uint32_t queueFamilyIndex_compute = VK_QUEUE_FAMILY_IGNORED;
        VkQueue queue_graphics;
        VkQueue queue_presentation;
        VkQueue queue_compute;

        VkSurfaceKHR surface;
        std::vector <VkSurfaceFormatKHR> availableSurfaceFormats;

        VkSwapchainKHR swapchain;
        std::vector <VkImage> swapchainImages;
        std::vector <VkImageView> swapchainImageViews;
        uint32_t currentImageIndex = 0;
        VkSwapchainCreateInfoKHR swapchainCreateInfo = {};

        std::vector<const char*> instanceLayers;
        std::vector<const char*> instanceExtensions;
        std::vector<const char*> deviceExtensions;

        VkDebugUtilsMessengerEXT debugMessenger;

        std::vector<std::function<void()>> callbacks_createSwapchain;
        std::vector<std::function<void()>> callbacks_destroySwapchain;
        std::vector<void(*)()> callbacks_createDevice;
        std::vector<void(*)()> callbacks_destroyDevice;

        graphicsBasePlus* pPlus = nullptr;
        //Static
        static graphicsBase singleton;
        //--------------------
        graphicsBase() = default;
        graphicsBase(graphicsBase&&) = delete;
        ~graphicsBase() {
            if (!instance)
                return;
            if (device) {
                WaitIdle();
                if (swapchain) {
                    for (auto& i : callbacks_destroySwapchain)
                        i();
                    for (auto& i : swapchainImageViews)
                        if (i)
                            vkDestroyImageView(device, i, nullptr);
                    vkDestroySwapchainKHR(device, swapchain, nullptr);
                }
                for (auto& i : callbacks_destroyDevice)
                    i();
                vkDestroyDevice(device, nullptr);
            }
            if (surface)
                vkDestroySurfaceKHR(instance, surface, nullptr);
            if (debugMessenger) {
                PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessenger =
                        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
                if (DestroyDebugUtilsMessenger)
                    DestroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
            }
            vkDestroyInstance(instance, nullptr);
        }
        //Non-const Function
        result_t CreateSwapchain_Internal() {
            if (VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain)) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to create a swapchain!\nError code: {}\n", int32_t(result));
                return result;
            }

            uint32_t swapchainImageCount;
            if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr)) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get the count of swapchain images!\nError code: {}\n", int32_t(result));
                return result;
            }
            swapchainImages.resize(swapchainImageCount);
            if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data())) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get swapchain images!\nError code: {}\n", int32_t(result));
                return result;
            }

            swapchainImageViews.resize(swapchainImageCount);
            VkImageViewCreateInfo imageViewCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = swapchainCreateInfo.imageFormat,
                    //.components = {},
                    .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
            };
            for (size_t i = 0; i < swapchainImageCount; i++) {
                imageViewCreateInfo.image = swapchainImages[i];
                if (VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i])) {
                    outStream << std::format("[ graphicsBase ] ERROR\nFailed to create a swapchain image view!\nError code: {}\n", int32_t(result));
                    return result;
                }
            }
            return VK_SUCCESS;
        }
        result_t GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t(&queueFamilyIndices)[3]) {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
            if (!queueFamilyCount)
                return VK_RESULT_MAX_ENUM;
            std::vector<VkQueueFamilyProperties> queueFamilyPropertieses(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyPropertieses.data());
            auto& [ig, ip, ic] = queueFamilyIndices;
            ig = ip = ic = VK_QUEUE_FAMILY_IGNORED;
            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                VkBool32
                        supportGraphics = enableGraphicsQueue && queueFamilyPropertieses[i].queueFlags & VK_QUEUE_GRAPHICS_BIT,
                        supportPresentation = false,
                        supportCompute = enableComputeQueue && queueFamilyPropertieses[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
                if (surface)
                    if (VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresentation)) {
                        outStream << std::format("[ graphicsBase ] ERROR\nFailed to determine if the queue family supports presentation!\nError code: {}\n", int32_t(result));
                        return result;
                    }
                if (supportGraphics && supportCompute) {
                    if (supportPresentation) {
                        ig = ip = ic = i;
                        break;
                    }
                    if (ig != ic ||
                        ig == VK_QUEUE_FAMILY_IGNORED)
                        ig = ic = i;
                    if (!surface)
                        break;
                }
                if (supportGraphics &&
                    ig == VK_QUEUE_FAMILY_IGNORED)
                    ig = i;
                if (supportPresentation &&
                    ip == VK_QUEUE_FAMILY_IGNORED)
                    ip = i;
                if (supportCompute &&
                    ic == VK_QUEUE_FAMILY_IGNORED)
                    ic = i;
            }
            if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
                ip == VK_QUEUE_FAMILY_IGNORED && surface ||
                ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue)
                return VK_RESULT_MAX_ENUM;
            queueFamilyIndex_graphics = ig;
            queueFamilyIndex_presentation = ip;
            queueFamilyIndex_compute = ic;
            return VK_SUCCESS;
        }
        result_t CreateDebugMessenger() {
            static PFN_vkDebugUtilsMessengerCallbackEXT DebugUtilsMessengerCallback = [](
                    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                    void* pUserData)->VkBool32 {
                outStream << std::format("{}\n\n", pCallbackData->pMessage);
                return VK_FALSE;
            };
            VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                    .messageSeverity =
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                    .messageType =
                    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                    .pfnUserCallback = DebugUtilsMessengerCallback
            };
            PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger =
                    reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
            if (vkCreateDebugUtilsMessenger) {
                VkResult result = vkCreateDebugUtilsMessenger(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugMessenger);
                if (result)
                    outStream << std::format("[ graphicsBase ] ERROR\nFailed to create a debug messenger!\nError code: {}\n", int32_t(result));
                return result;
            }
            outStream << std::format("[ graphicsBase ] ERROR\nFailed to get the function pointer of vkCreateDebugUtilsMessengerEXT!\n");
            return VK_RESULT_MAX_ENUM;
        }
        //Static Function
        static void AddLayerOrExtension(std::vector<const char*>& container, const char* name) {
            for (auto& i : container)
                if (!strcmp(name, i))
                    return;
            container.push_back(name);
        }
    public:
        //Getter
        uint32_t ApiVersion() const {
            return apiVersion;
        }
        VkInstance Instance() const {
            return instance;
        }
        VkPhysicalDevice PhysicalDevice() const {
            return physicalDevice;
        }
        const VkPhysicalDeviceProperties& PhysicalDeviceProperties() const {
            return physicalDeviceProperties;
        }
        const VkPhysicalDeviceMemoryProperties& PhysicalDeviceMemoryProperties() const {
            return physicalDeviceMemoryProperties;
        }
        VkPhysicalDevice AvailablePhysicalDevice(uint32_t index) const {
            return availablePhysicalDevices[index];
        }
        uint32_t AvailablePhysicalDeviceCount() const {
            return uint32_t(availablePhysicalDevices.size());
        }

        VkDevice Device() const {
            return device;
        }
        uint32_t QueueFamilyIndex_Graphics() const {
            return queueFamilyIndex_graphics;
        }
        uint32_t QueueFamilyIndex_Presentation() const {
            return queueFamilyIndex_presentation;
        }
        uint32_t QueueFamilyIndex_Compute() const {
            return queueFamilyIndex_compute;
        }
        VkQueue Queue_Graphics() const {
            return queue_graphics;
        }
        VkQueue Queue_Presentation() const {
            return queue_presentation;
        }
        VkQueue Queue_Compute() const {
            return queue_compute;
        }

        VkSurfaceKHR Surface() const {
            return surface;
        }
        const VkFormat& AvailableSurfaceFormat(uint32_t index) const {
            return availableSurfaceFormats[index].format;
        }
        const VkColorSpaceKHR& AvailableSurfaceColorSpace(uint32_t index) const {
            return availableSurfaceFormats[index].colorSpace;
        }
        uint32_t AvailableSurfaceFormatCount() const {
            return uint32_t(availableSurfaceFormats.size());
        }

        VkSwapchainKHR Swapchain() const {
            return swapchain;
        }
        VkImage SwapchainImage(uint32_t index) const {
            return swapchainImages[index];
        }
        VkImageView SwapchainImageView(uint32_t index) const {
            return swapchainImageViews[index];
        }
        uint32_t SwapchainImageCount() const {
            return uint32_t(swapchainImages.size());
        }
        uint32_t CurrentImageIndex() const { return currentImageIndex; }
        const VkSwapchainCreateInfoKHR& SwapchainCreateInfo() const {
            return swapchainCreateInfo;
        }

        const std::vector<const char*>& InstanceLayers() const {
            return instanceLayers;
        }
        const std::vector<const char*>& InstanceExtensions() const {
            return instanceExtensions;
        }
        const std::vector<const char*>& DeviceExtensions() const {
            return deviceExtensions;
        }

        //Const Function
        VkResult WaitIdle() const {
            VkResult result = vkDeviceWaitIdle(device);
            if (result)
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to wait for the device to be idle!\nError code: {}\n", int32_t(result));
            return result;
        }

        //Non-const Function
        void AddCallback_CreateSwapchain(std::function<void()> function) {
            callbacks_createSwapchain.push_back(function);
        }
        void AddCallback_DestroySwapchain(std::function<void()> function) {
            callbacks_destroySwapchain.push_back(function);
        }
        void AddCallback_CreateDevice(void(*function)()) {
            callbacks_createDevice.push_back(function);
        }
        void AddCallback_DestroyDevice(void(*function)()) {
            callbacks_destroyDevice.push_back(function);
        }
        //                    Create Instance
        void AddInstanceLayer(const char* layerName) {
            AddLayerOrExtension(instanceLayers, layerName);
        }
        void AddInstanceExtension(const char* extensionName) {
            AddLayerOrExtension(instanceExtensions, extensionName);
        }
        result_t UseLatestApiVersion() {
            if (vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"))
                return vkEnumerateInstanceVersion(&apiVersion);
            return VK_SUCCESS;
        }
        result_t CreateInstance(VkInstanceCreateFlags flags = 0) {
            if constexpr (ENABLE_DEBUG_MESSENGER)
                AddInstanceLayer("VK_LAYER_KHRONOS_validation"),
                        AddInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            VkApplicationInfo applicatianInfo = {
                    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                    .apiVersion = apiVersion
            };
            VkInstanceCreateInfo instanceCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                    .flags = flags,
                    .pApplicationInfo = &applicatianInfo,
                    .enabledLayerCount = uint32_t(instanceLayers.size()),
                    .ppEnabledLayerNames = instanceLayers.data(),
                    .enabledExtensionCount = uint32_t(instanceExtensions.size()),
                    .ppEnabledExtensionNames = instanceExtensions.data()
            };
            if (VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance)) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to create a vulkan instance!\nError code: {}\n", int32_t(result));
                return result;
            }
            outStream << std::format(
                    "Vulkan API Version: {}.{}.{}\n",
                    VK_VERSION_MAJOR(apiVersion),
                    VK_VERSION_MINOR(apiVersion),
                    VK_VERSION_PATCH(apiVersion));
            if constexpr (ENABLE_DEBUG_MESSENGER)
                CreateDebugMessenger();
            return VK_SUCCESS;
        }
        result_t CheckInstanceLayers(arrayRef<const char*> layersToCheck) const {
            uint32_t layerCount;
            std::vector<VkLayerProperties> availableLayers;
            if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr)) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get the count of instance layers!\n");
                return result;
            }
            if (layerCount) {
                availableLayers.resize(layerCount);
                if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data())) {
                    outStream << std::format("[ graphicsBase ] ERROR\nFailed to enumerate instance layer properties!\nError code: {}\n", int32_t(result));
                    return result;
                }
                for (auto& i : layersToCheck) {
                    bool found = false;
                    for (auto& j : availableLayers)
                        if (!strcmp(i, j.layerName)) {
                            found = true;
                            break;
                        }
                    if (!found)
                        i = nullptr;
                }
            }
            else
                for (auto& i : layersToCheck)
                    i = nullptr;
            return VK_SUCCESS;
        }
        result_t CheckInstanceExtensions(arrayRef<const char*> extensionsToCheck, const char* layerName) const {
            uint32_t extensionCount;
            std::vector<VkExtensionProperties> availableExtensions;
            if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr)) {
                layerName ?
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\nLayer name:{}\n", layerName) :
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\n");
                return result;
            }
            if (extensionCount) {
                availableExtensions.resize(extensionCount);
                if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, availableExtensions.data())) {
                    outStream << std::format("[ graphicsBase ] ERROR\nFailed to enumerate instance extension properties!\nError code: {}\n", int32_t(result));
                    return result;
                }
                for (auto& i : extensionsToCheck) {
                    bool found = false;
                    for (auto& j : availableExtensions)
                        if (!strcmp(i, j.extensionName)) {
                            found = true;
                            break;
                        }
                    if (!found)
                        i = nullptr;
                }
            }
            else
                for (auto& i : extensionsToCheck)
                    i = nullptr;
            return VK_SUCCESS;
        }
        void InstanceLayers(const std::vector<const char*>& layerNames) {
            instanceLayers = layerNames;
        }
        void InstanceExtensions(const std::vector<const char*>& extensionNames) {
            instanceExtensions = extensionNames;
        }
        //                    Set Window Surface
        void Surface(VkSurfaceKHR surface) {
            if (!this->surface)
                this->surface = surface;
        }
        //                    Create Logical Device
        void AddDeviceExtension(const char* extensionName) {
            AddLayerOrExtension(deviceExtensions, extensionName);
        }
        result_t GetPhysicalDevices() {
            uint32_t deviceCount;
            if (VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr)) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get the count of physical devices!\nError code: {}\n", int32_t(result));
                return result;
            }
            if (!deviceCount)
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to find any physical device supports vulkan!\n"),
                        abort();
            availablePhysicalDevices.resize(deviceCount);
            VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data());
            if (result)
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to enumerate physical devices!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t DeterminePhysicalDevice(uint32_t deviceIndex = 0, bool enableGraphicsQueue = true, bool enableComputeQueue = true) {
            static constexpr uint32_t notFound = INT32_MAX;//== VK_QUEUE_FAMILY_IGNORED & INT32_MAX
            struct queueFamilyIndexCombination {
                uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
                uint32_t presentation = VK_QUEUE_FAMILY_IGNORED;
                uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
            };
            static std::vector<queueFamilyIndexCombination> queueFamilyIndexCombinations(availablePhysicalDevices.size());
            auto& [ig, ip, ic] = queueFamilyIndexCombinations[deviceIndex];
            if (ig == notFound && enableGraphicsQueue ||
                ip == notFound && surface ||
                ic == notFound && enableComputeQueue)
                return VK_RESULT_MAX_ENUM;
            if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
                ip == VK_QUEUE_FAMILY_IGNORED && surface ||
                ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue) {
                uint32_t indices[3];
                VkResult result = GetQueueFamilyIndices(availablePhysicalDevices[deviceIndex], enableGraphicsQueue, enableComputeQueue, indices);
                if (result == VK_SUCCESS ||
                    result == VK_RESULT_MAX_ENUM) {
                    if (enableGraphicsQueue)
                        ig = indices[0] & INT32_MAX;
                    if (surface)
                        ip = indices[1] & INT32_MAX;
                    if (enableComputeQueue)
                        ic = indices[2] & INT32_MAX;
                }
                if (result)
                    return result;
            }
            else {
                queueFamilyIndex_graphics = enableGraphicsQueue ? ig : VK_QUEUE_FAMILY_IGNORED;
                queueFamilyIndex_presentation = surface ? ip : VK_QUEUE_FAMILY_IGNORED;
                queueFamilyIndex_compute = enableComputeQueue ? ic : VK_QUEUE_FAMILY_IGNORED;
            }
            physicalDevice = availablePhysicalDevices[deviceIndex];
            return VK_SUCCESS;
        }
        result_t CreateDevice(VkDeviceCreateFlags flags = 0) {
            float queuePriority = 1.f;
            VkDeviceQueueCreateInfo queueCreateInfos[3] = {
                    {
                            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                            .queueCount = 1,
                            .pQueuePriorities = &queuePriority },
                    {
                            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                            .queueCount = 1,
                            .pQueuePriorities = &queuePriority },
                    {
                            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                            .queueCount = 1,
                            .pQueuePriorities = &queuePriority } };
            uint32_t queueCreateInfoCount = 0;
            if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
                queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_graphics;
            if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED &&
                queueFamilyIndex_presentation != queueFamilyIndex_graphics)
                queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_presentation;
            if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED &&
                queueFamilyIndex_compute != queueFamilyIndex_graphics &&
                queueFamilyIndex_compute != queueFamilyIndex_presentation)
                queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_compute;
            VkPhysicalDeviceFeatures physicalDeviceFeatures;
            vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
            VkDeviceCreateInfo deviceCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                    .flags = flags,
                    .queueCreateInfoCount = queueCreateInfoCount,
                    .pQueueCreateInfos = queueCreateInfos,
                    .enabledExtensionCount = uint32_t(deviceExtensions.size()),
                    .ppEnabledExtensionNames = deviceExtensions.data(),
                    .pEnabledFeatures = &physicalDeviceFeatures
            };
            if (VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device)) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to create a vulkan logical device!\nError code: {}\n", int32_t(result));
                return result;
            }
            if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
                vkGetDeviceQueue(device, queueFamilyIndex_graphics, 0, &queue_graphics);
            if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED)
                vkGetDeviceQueue(device, queueFamilyIndex_presentation, 0, &queue_presentation);
            if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED)
                vkGetDeviceQueue(device, queueFamilyIndex_compute, 0, &queue_compute);
            vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
            outStream << std::format("Renderer: {}\n", physicalDeviceProperties.deviceName);
            return VK_SUCCESS;
        }
        result_t CheckDeviceExtensions(arrayRef<const char*> extensionsToCheck, const char* layerName = nullptr) const {
            uint32_t extensionCount;
            std::vector<VkExtensionProperties> availableExtensions;
            if (VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, layerName, &extensionCount, nullptr)) {
                layerName ?
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get the count of device extensions!\nLayer name:{}\n", layerName) :
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get the count of device extensions!\n");
                return result;
            }
            if (extensionCount) {
                availableExtensions.resize(extensionCount);
                if (VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, layerName, &extensionCount, availableExtensions.data())) {
                    outStream << std::format("[ graphicsBase ] ERROR\nFailed to enumerate device extension properties!\nError code: {}\n", int32_t(result));
                    return result;
                }
                for (auto& i : extensionsToCheck) {
                    bool found = false;
                    for (auto& j : availableExtensions)
                        if (!strcmp(i, j.extensionName)) {
                            found = true;
                            break;
                        }
                    if (!found)
                        i = nullptr;//If a required extension isn't available, set it to nullptr
                }
            }
            else
                for (auto& i : extensionsToCheck)
                    i = nullptr;
            return VK_SUCCESS;
        }
        void DeviceExtensions(const std::vector<const char*>& extensionNames) {
            deviceExtensions = extensionNames;
        }
        //                    Create Swapchain
        result_t GetSurfaceFormats() {
            uint32_t surfaceFormatCount;
            if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr)) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get the count of surface formats!\nError code: {}\n", int32_t(result));
                return result;
            }
            if (!surfaceFormatCount)
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to find any supported surface format!\n"),
                        abort();
            availableSurfaceFormats.resize(surfaceFormatCount);
            VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, availableSurfaceFormats.data());
            if (result)
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get surface formats!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat) {
            bool formatIsAvailable = false;
            if (!surfaceFormat.format) {
                for (auto& i : availableSurfaceFormats)
                    if (i.colorSpace == surfaceFormat.colorSpace) {
                        swapchainCreateInfo.imageFormat = i.format;
                        swapchainCreateInfo.imageColorSpace = i.colorSpace;
                        formatIsAvailable = true;
                        break;
                    }
            }
            else
                for (auto& i : availableSurfaceFormats)
                    if (i.format == surfaceFormat.format &&
                        i.colorSpace == surfaceFormat.colorSpace) {
                        swapchainCreateInfo.imageFormat = i.format;
                        swapchainCreateInfo.imageColorSpace = i.colorSpace;
                        formatIsAvailable = true;
                        break;
                    }
            if (!formatIsAvailable)
                return VK_ERROR_FORMAT_NOT_SUPPORTED;
            if (swapchain)
                return RecreateSwapchain();
            return VK_SUCCESS;
        }
        result_t CreateSwapchain(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0) {
            //Get surface capabilities
            VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
            if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities)) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
                return result;
            }
            //Set image count
            swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.maxImageCount > surfaceCapabilities.minImageCount);
            //Set image extent
            swapchainCreateInfo.imageExtent =
                    surfaceCapabilities.currentExtent.width == -1 ?
                    VkExtent2D{
                            glm::clamp(defaultWindowSize.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
                            glm::clamp(defaultWindowSize.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height) } :
                    surfaceCapabilities.currentExtent;
            //Set transformation
            swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
            //Set alpha compositing mode
            if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
                swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
            else
                for (size_t i = 0; i < 4; i++)
                    if (surfaceCapabilities.supportedCompositeAlpha & 1 << i) {
                        swapchainCreateInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR(surfaceCapabilities.supportedCompositeAlpha & 1 << i);
                        break;
                    }
            //Set image usage
            swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            else
                outStream << std::format("[ graphicsBase ] WARNING\nVK_IMAGE_USAGE_TRANSFER_DST_BIT isn't supported!\n");

            //Get surface formats
            if (!availableSurfaceFormats.size())
                if (VkResult result = GetSurfaceFormats())
                    return result;
            //If surface format is not determined, select a a four-component UNORM format
            if (!swapchainCreateInfo.imageFormat)
                if (SetSurfaceFormat({ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }) &&
                    SetSurfaceFormat({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })) {
                    swapchainCreateInfo.imageFormat = availableSurfaceFormats[0].format;
                    swapchainCreateInfo.imageColorSpace = availableSurfaceFormats[0].colorSpace;
                    outStream << std::format("[ graphicsBase ] WARNING\nFailed to select a four-component UNORM surface format!\n");
                }

            //Get surface present modes
            uint32_t surfacePresentModeCount;
            if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, nullptr)) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get the count of surface present modes!\nError code: {}\n", int32_t(result));
                return result;
            }
            if (!surfacePresentModeCount)
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to find any surface present mode!\n"),
                        abort();
            std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
            if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, surfacePresentModes.data())) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get surface present modes!\nError code: {}\n", int32_t(result));
                return result;
            }
            //Set present mode to mailbox if available and necessary
            swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
            if (!limitFrameRate)
                for (size_t i = 0; i < surfacePresentModeCount; i++)
                    if (surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                        swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                        break;
                    }

            swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainCreateInfo.flags = flags;
            swapchainCreateInfo.surface = surface;
            swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.clipped = VK_TRUE;

            if (VkResult result = CreateSwapchain_Internal())
                return result;
            for (auto& i : callbacks_createSwapchain)
                i();
            return VK_SUCCESS;
        }

        //                    After initialization
        void Terminate() {
            this->~graphicsBase();
            instance = VK_NULL_HANDLE;
            physicalDevice = VK_NULL_HANDLE;
            device = VK_NULL_HANDLE;
            surface = VK_NULL_HANDLE;
            swapchain = VK_NULL_HANDLE;
            swapchainImages.resize(0);
            swapchainImageViews.resize(0);
            swapchainCreateInfo = {};
            debugMessenger = VK_NULL_HANDLE;
        }
        result_t RecreateDevice(VkDeviceCreateFlags flags = 0) {
            if (VkResult result = WaitIdle())
                return result;
            if (swapchain) {
                for (auto& i : callbacks_destroySwapchain)
                    i();
                for (auto& i : swapchainImageViews)
                    if (i)
                        vkDestroyImageView(device, i, nullptr);
                swapchainImageViews.resize(0);
                vkDestroySwapchainKHR(device, swapchain, nullptr);
                swapchain = VK_NULL_HANDLE;
                swapchainCreateInfo = {};
            }
            for (auto& i : callbacks_destroyDevice)
                i();
            if (device)
                vkDestroyDevice(device, nullptr),
                        device = VK_NULL_HANDLE;
            return CreateDevice(flags);
        }
        result_t RecreateSwapchain() {
            VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
            if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities)) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
                return result;
            }
            if (surfaceCapabilities.currentExtent.width == 0 ||
                surfaceCapabilities.currentExtent.height == 0)
                return VK_SUBOPTIMAL_KHR;
            swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
            swapchainCreateInfo.oldSwapchain = swapchain;
            VkResult result = vkQueueWaitIdle(queue_graphics);
            if (!result &&
                queue_graphics != queue_presentation)
                result = vkQueueWaitIdle(queue_presentation);
            if (result) {
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to wait for the queue to be idle!\nError code: {}\n", int32_t(result));
                return result;
            }

            for (auto& i : callbacks_destroySwapchain)
                i();
            for (auto& i : swapchainImageViews)
                if (i)
                    vkDestroyImageView(device, i, nullptr);
            swapchainImageViews.resize(0);
            if (result = CreateSwapchain_Internal())
                return result;
            for (auto& i : callbacks_createSwapchain)
                i();
            return VK_SUCCESS;
        }
        result_t SwapImage(VkSemaphore semaphore_imageIsAvailable) {
            if (swapchainCreateInfo.oldSwapchain &&
                swapchainCreateInfo.oldSwapchain != swapchain) {
                vkDestroySwapchainKHR(device, swapchainCreateInfo.oldSwapchain, nullptr);
                swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
            }
            while (VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore_imageIsAvailable, VK_NULL_HANDLE, &currentImageIndex))
                switch (result) {
                    case VK_SUBOPTIMAL_KHR:
                    case VK_ERROR_OUT_OF_DATE_KHR:
                        if (VkResult result = RecreateSwapchain())
                            return result;
                        break;
                    default:
                        outStream << std::format("[ graphicsBase ] ERROR\nFailed to acquire the next image!\nError code: {}\n", int32_t(result));
                        return result;
                }
            return VK_SUCCESS;
        }
        result_t SubmitCommandBuffer_Graphics(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const {
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkResult result = vkQueueSubmit(queue_graphics, 1, &submitInfo, fence);
            if (result)
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer,
                                              VkSemaphore semaphore_imageIsAvailable = VK_NULL_HANDLE, VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE,
                                              VkPipelineStageFlags waitDstStage_imageIsAvailable = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) const {
            VkSubmitInfo submitInfo = {
                    .commandBufferCount = 1,
                    .pCommandBuffers = &commandBuffer
            };
            if (semaphore_imageIsAvailable)
                submitInfo.waitSemaphoreCount = 1,
                submitInfo.pWaitSemaphores = &semaphore_imageIsAvailable,
                submitInfo.pWaitDstStageMask = &waitDstStage_imageIsAvailable;
            if (semaphore_renderingIsOver)
                submitInfo.signalSemaphoreCount = 1,
                        submitInfo.pSignalSemaphores = &semaphore_renderingIsOver;
            return SubmitCommandBuffer_Graphics(submitInfo, fence);
        }
        result_t SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE) const {
            VkSubmitInfo submitInfo = {
                    .commandBufferCount = 1,
                    .pCommandBuffers = &commandBuffer
            };
            return SubmitCommandBuffer_Graphics(submitInfo, fence);
        }
        result_t SubmitCommandBuffer_Compute(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const {
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkResult result = vkQueueSubmit(queue_compute, 1, &submitInfo, fence);
            if (result)
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t SubmitCommandBuffer_Compute(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE) const {
            VkSubmitInfo submitInfo = {
                    .commandBufferCount = 1,
                    .pCommandBuffers = &commandBuffer
            };
            return SubmitCommandBuffer_Compute(submitInfo, fence);
        }
        result_t SubmitCommandBuffer_Presentation(VkCommandBuffer commandBuffer,
                                                  VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE, VkSemaphore semaphore_ownershipIsTransfered = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE) const {
            static constexpr VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            VkSubmitInfo submitInfo = {
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                    .commandBufferCount = 1,
                    .pCommandBuffers = &commandBuffer
            };
            if (semaphore_renderingIsOver)
                submitInfo.waitSemaphoreCount = 1,
                submitInfo.pWaitSemaphores = &semaphore_renderingIsOver,
                submitInfo.pWaitDstStageMask = &waitDstStage;
            if (semaphore_ownershipIsTransfered)
                submitInfo.signalSemaphoreCount = 1,
                        submitInfo.pSignalSemaphores = &semaphore_ownershipIsTransfered;
            VkResult result = vkQueueSubmit(queue_presentation, 1, &submitInfo, fence);
            if (result)
                outStream << std::format("[ graphicsBase ] ERROR\nFailed to submit the presentation command buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
        void CmdTransferImageOwnership(VkCommandBuffer commandBuffer) const {
            VkImageMemoryBarrier imageMemoryBarrier_g2p = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = 0,
                    .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .srcQueueFamilyIndex = queueFamilyIndex_graphics,
                    .dstQueueFamilyIndex = queueFamilyIndex_presentation,
                    .image = swapchainImages[currentImageIndex],
                    .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
            };
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
                                 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier_g2p);
        }
        result_t PresentImage(VkPresentInfoKHR& presentInfo) {
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            switch (VkResult result = vkQueuePresentKHR(queue_presentation, &presentInfo)) {
                case VK_SUCCESS:
                    return VK_SUCCESS;
                case VK_SUBOPTIMAL_KHR:
                case VK_ERROR_OUT_OF_DATE_KHR:
                    return RecreateSwapchain();
                default:
                    outStream << std::format("[ graphicsBase ] ERROR\nFailed to queue the image for presentation!\nError code: {}\n", int32_t(result));
                    return result;
            }
        }
        result_t PresentImage(VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE) {
            VkPresentInfoKHR presentInfo = {
                    .swapchainCount = 1,
                    .pSwapchains = &swapchain,
                    .pImageIndices = &currentImageIndex
            };
            if (semaphore_renderingIsOver)
                presentInfo.waitSemaphoreCount = 1,
                        presentInfo.pWaitSemaphores = &semaphore_renderingIsOver;
            return PresentImage(presentInfo);
        }

        //Static Function
        static graphicsBase& Base() {
            return singleton;
        }
        static graphicsBasePlus& Plus() { return *singleton.pPlus; }
        static void Plus(graphicsBasePlus& plus) { if (!singleton.pPlus) singleton.pPlus = &plus; }

    };
    inline graphicsBase graphicsBase::singleton;

    class semaphore {
        VkSemaphore handle = VK_NULL_HANDLE;
    public:
        //semaphore() = default;
        semaphore(VkSemaphoreCreateInfo& createInfo) {
            Create(createInfo);
        }
        semaphore(/*reserved for future use*/) {
            Create();
        }
        semaphore(semaphore&& other) noexcept { MoveHandle; }
        ~semaphore() { DestroyHandleBy(vkDestroySemaphore); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Non-const Function
        result_t Create(VkSemaphoreCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VkResult result = vkCreateSemaphore(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ semaphore ] ERROR\nFailed to create a semaphore!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t Create(/*reserved for future use*/) {
            VkSemaphoreCreateInfo createInfo = {};
            return Create(createInfo);
        }
    };
    class fence {
        VkFence handle = VK_NULL_HANDLE;
    public:
        //fence() = default;
        fence(VkFenceCreateInfo& createInfo) {
            Create(createInfo);
        }
        fence(VkFenceCreateFlags flags = 0) {
            Create(flags);
        }
        fence(fence&& other) noexcept { MoveHandle; }
        ~fence() { DestroyHandleBy(vkDestroyFence); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Const Function
        result_t Wait() const {
            VkResult result = vkWaitForFences(graphicsBase::Base().Device(), 1, &handle, false, UINT64_MAX);
            if (result)
                outStream << std::format("[ fence ] ERROR\nFailed to wait for the fence!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t Reset() const {
            VkResult result = vkResetFences(graphicsBase::Base().Device(), 1, &handle);
            if (result)
                outStream << std::format("[ fence ] ERROR\nFailed to reset the fence!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t WaitAndReset() const {
            VkResult result = Wait();
            result || (result = Reset());
            return result;
        }
        result_t Status() const {
            VkResult result = vkGetFenceStatus(graphicsBase::Base().Device(), handle);
            if (result < 0)
                outStream << std::format("[ fence ] ERROR\nFailed to get the status of the fence!\nError code: {}\n", int32_t(result));
            return result;
        }
        //Non-const Function
        result_t Create(VkFenceCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            VkResult result = vkCreateFence(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ fence ] ERROR\nFailed to create a fence!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t Create(VkFenceCreateFlags flags = 0) {
            VkFenceCreateInfo createInfo = {
                    .flags = flags
            };
            return Create(createInfo);
        }
    };
    class sampler {
        VkSampler handle = VK_NULL_HANDLE;
    public:
        sampler() = default;
        sampler(VkSamplerCreateInfo& createInfo) {
            Create(createInfo);
        }
        sampler(sampler&& other) noexcept { MoveHandle; }
        ~sampler() { DestroyHandleBy(vkDestroySampler); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Non-const Function
        result_t Create(VkSamplerCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            VkResult result = vkCreateSampler(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ sampler ] ERROR\nFailed to create a sampler!\nError code: {}\n", int32_t(result));
            return result;
        }
    };

    class shaderModule {
        VkShaderModule handle = VK_NULL_HANDLE;
    public:
        shaderModule() = default;
        shaderModule(VkShaderModuleCreateInfo& createInfo) {
            Create(createInfo);
        }
        shaderModule(const char* filepath /*reserved for future use*/) {
            Create(filepath);
        }
        shaderModule(size_t codeSize, const uint32_t* pCode /*reserved for future use*/) {
            Create(codeSize, pCode);
        }
        shaderModule(shaderModule&& other) noexcept { MoveHandle; }
        ~shaderModule() { DestroyHandleBy(vkDestroyShaderModule); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Const Function
        VkPipelineShaderStageCreateInfo StageCreateInfo(VkShaderStageFlagBits stage, const char* entry = "main") const {
            return {
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,//sType
                    nullptr,                                            //pNext
                    0,                                                  //flags
                    stage,                                              //stage
                    handle,                                             //module
                    entry,                                              //pName
                    nullptr                                             //pSpecializationInfo
            };
        }
        //Non-const Function
        result_t Create(VkShaderModuleCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            VkResult result = vkCreateShaderModule(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ shader ] ERROR\nFailed to create a shader module!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t Create(const char* filepath /*reserved for future use*/) {
            std::ifstream file(filepath, std::ios::ate | std::ios::binary);
            if (!file) {
                outStream << std::format("[ shader ] ERROR\nFailed to open the file: {}\n", filepath);
                return VK_RESULT_MAX_ENUM;//No proper VkResult enum value, don't use VK_ERROR_UNKNOWN
            }
            size_t fileSize = size_t(file.tellg());
            std::vector<uint32_t> binaries(fileSize / 4);
            file.seekg(0);
            file.read(reinterpret_cast<char*>(binaries.data()), fileSize);
            file.close();
            return Create(fileSize, binaries.data());
        }
        result_t Create(size_t codeSize, const uint32_t* pCode /*reserved for future use*/) {
            VkShaderModuleCreateInfo createInfo = {
                    .codeSize = codeSize,
                    .pCode = pCode
            };
            return Create(createInfo);
        }
    };
    class pipelineLayout {
        VkPipelineLayout handle = VK_NULL_HANDLE;
    public:
        pipelineLayout() = default;
        pipelineLayout(VkPipelineLayoutCreateInfo& createInfo) {
            Create(createInfo);
        }
        pipelineLayout(pipelineLayout&& other) noexcept { MoveHandle; }
        ~pipelineLayout() { DestroyHandleBy(vkDestroyPipelineLayout); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Non-const Function
        result_t Create(VkPipelineLayoutCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            VkResult result = vkCreatePipelineLayout(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ pipelineLayout ] ERROR\nFailed to create a pipeline layout!\nError code: {}\n", int32_t(result));
            return result;
        }
    };
    class pipeline {
        VkPipeline handle = VK_NULL_HANDLE;
    public:
        pipeline() = default;
        pipeline(VkGraphicsPipelineCreateInfo& createInfo) {
            Create(createInfo);
        }
        pipeline(VkComputePipelineCreateInfo& createInfo) {
            Create(createInfo);
        }
        pipeline(pipeline&& other) noexcept { MoveHandle; }
        ~pipeline() { DestroyHandleBy(vkDestroyPipeline); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Non-const Function
        result_t Create(VkGraphicsPipelineCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            VkResult result = vkCreateGraphicsPipelines(graphicsBase::Base().Device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ pipeline ] ERROR\nFailed to create a graphics pipeline!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t Create(VkComputePipelineCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            VkResult result = vkCreateComputePipelines(graphicsBase::Base().Device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ pipeline ] ERROR\nFailed to create a compute pipeline!\nError code: {}\n", int32_t(result));
            return result;
        }
    };

    class renderPass {
        VkRenderPass handle = VK_NULL_HANDLE;
    public:
        renderPass() = default;
        renderPass(VkRenderPassCreateInfo& createInfo) {
            Create(createInfo);
        }
        renderPass(renderPass&& other) noexcept { MoveHandle; }
        ~renderPass() { DestroyHandleBy(vkDestroyRenderPass); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Const Function
        void CmdBegin(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo& beginInfo, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
            beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            beginInfo.renderPass = handle;
            vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
        }
        void CmdBegin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRect2D renderArea, arrayRef<const VkClearValue> clearValues = {}, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
            VkRenderPassBeginInfo beginInfo = {
                    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                    .renderPass = handle,
                    .framebuffer = framebuffer,
                    .renderArea = renderArea,
                    .clearValueCount = uint32_t(clearValues.Count()),
                    .pClearValues = clearValues.Pointer()
            };
            vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
        }
        void CmdNext(VkCommandBuffer commandBuffer, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
            vkCmdNextSubpass(commandBuffer, subpassContents);
        }
        void CmdEnd(VkCommandBuffer commandBuffer) const {
            vkCmdEndRenderPass(commandBuffer);
        }
        //Non-const Function
        result_t Create(VkRenderPassCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            VkResult result = vkCreateRenderPass(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ renderPass ] ERROR\nFailed to create a render pass!\nError code: {}\n", int32_t(result));
            return result;
        }
    };
    class framebuffer {
        VkFramebuffer handle = VK_NULL_HANDLE;
    public:
        framebuffer() = default;
        framebuffer(VkFramebufferCreateInfo& createInfo) {
            Create(createInfo);
        }
        framebuffer(framebuffer&& other) noexcept { MoveHandle; }
        ~framebuffer() { DestroyHandleBy(vkDestroyFramebuffer); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Non-const Function
        result_t Create(VkFramebufferCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            VkResult result = vkCreateFramebuffer(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ framebuffer ] ERROR\nFailed to create a framebuffer!\nError code: {}\n", int32_t(result));
            return result;
        }
    };

    class commandBuffer {
        friend class commandPool;
        VkCommandBuffer handle = VK_NULL_HANDLE;
    public:
        commandBuffer() = default;
        commandBuffer(commandBuffer&& other) noexcept { MoveHandle; }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
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
    class commandPool {
        VkCommandPool handle = VK_NULL_HANDLE;
    public:
        commandPool() = default;
        commandPool(VkCommandPoolCreateInfo& createInfo) {
            Create(createInfo);
        }
        commandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
            Create(queueFamilyIndex, flags);
        }
        commandPool(commandPool&& other) noexcept { MoveHandle; }
        ~commandPool() { DestroyHandleBy(vkDestroyCommandPool); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Const Function
        result_t AllocateBuffers(arrayRef<VkCommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
            VkCommandBufferAllocateInfo allocateInfo = {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .commandPool = handle,
                    .level = level,
                    .commandBufferCount = uint32_t(buffers.Count())
            };
            VkResult result = vkAllocateCommandBuffers(graphicsBase::Base().Device(), &allocateInfo, buffers.Pointer());
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
            vkFreeCommandBuffers(graphicsBase::Base().Device(), handle, buffers.Count(), buffers.Pointer());
            memset(buffers.Pointer(), 0, buffers.Count() * sizeof(VkCommandBuffer));
        }
        void FreeBuffers(arrayRef<commandBuffer> buffers) const {
            FreeBuffers({ &buffers[0].handle, buffers.Count() });
        }
        //Non-const Function
        result_t Create(VkCommandPoolCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            VkResult result = vkCreateCommandPool(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
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
    struct graphicsPipelineCreateInfoPack {
        VkGraphicsPipelineCreateInfo createInfo =
                { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        //Vertex Input
        VkPipelineVertexInputStateCreateInfo vertexInputStateCi =
                { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        std::vector<VkVertexInputBindingDescription> vertexInputBindings;
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
        //Input Assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCi =
                { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        //Tessellation
        VkPipelineTessellationStateCreateInfo tessellationStateCi =
                { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
        //Viewport
        VkPipelineViewportStateCreateInfo viewportStateCi =
                { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        std::vector<VkViewport> viewports;
        std::vector<VkRect2D> scissors;
        uint32_t dynamicViewportCount = 1;
        uint32_t dynamicScissorCount = 1;
        //Rasterization
        VkPipelineRasterizationStateCreateInfo rasterizationStateCi =
                { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        //Multisample
        VkPipelineMultisampleStateCreateInfo multisampleStateCi =
                { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        //Depth & Stencil
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCi =
                { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        //Color Blend
        VkPipelineColorBlendStateCreateInfo colorBlendStateCi =
                { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
        //Dynamic
        VkPipelineDynamicStateCreateInfo dynamicStateCi =
                { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        std::vector<VkDynamicState> dynamicStates;
        //--------------------
        graphicsPipelineCreateInfoPack() {
            SetCreateInfos();
            createInfo.basePipelineIndex = -1;
        }
        graphicsPipelineCreateInfoPack(const graphicsPipelineCreateInfoPack& other) noexcept {
            createInfo = other.createInfo;
            SetCreateInfos();

            vertexInputStateCi = other.vertexInputStateCi;
            inputAssemblyStateCi = other.inputAssemblyStateCi;
            tessellationStateCi = other.tessellationStateCi;
            viewportStateCi = other.viewportStateCi;
            rasterizationStateCi = other.rasterizationStateCi;
            multisampleStateCi = other.multisampleStateCi;
            depthStencilStateCi = other.depthStencilStateCi;
            colorBlendStateCi = other.colorBlendStateCi;
            dynamicStateCi = other.dynamicStateCi;

            shaderStages = other.shaderStages;
            vertexInputBindings = other.vertexInputBindings;
            vertexInputAttributes = other.vertexInputAttributes;
            viewports = other.viewports;
            scissors = other.scissors;
            colorBlendAttachmentStates = other.colorBlendAttachmentStates;
            dynamicStates = other.dynamicStates;
            UpdateAllArrayAddresses();
        }
        //Getter
        operator VkGraphicsPipelineCreateInfo& () { return createInfo; }
        //Non-const Function
        void UpdateAllArrays() {
            createInfo.stageCount = shaderStages.size();
            vertexInputStateCi.vertexBindingDescriptionCount = vertexInputBindings.size();
            vertexInputStateCi.vertexAttributeDescriptionCount = vertexInputAttributes.size();
            viewportStateCi.viewportCount = viewports.size() ? uint32_t(viewports.size()) : dynamicViewportCount;
            viewportStateCi.scissorCount = scissors.size() ? uint32_t(scissors.size()) : dynamicScissorCount;
            colorBlendStateCi.attachmentCount = colorBlendAttachmentStates.size();
            dynamicStateCi.dynamicStateCount = dynamicStates.size();
            UpdateAllArrayAddresses();
        }
    private:
        void SetCreateInfos() {
            createInfo.pVertexInputState = &vertexInputStateCi;
            createInfo.pInputAssemblyState = &inputAssemblyStateCi;
            createInfo.pTessellationState = &tessellationStateCi;
            createInfo.pViewportState = &viewportStateCi;
            createInfo.pRasterizationState = &rasterizationStateCi;
            createInfo.pMultisampleState = &multisampleStateCi;
            createInfo.pDepthStencilState = &depthStencilStateCi;
            createInfo.pColorBlendState = &colorBlendStateCi;
            createInfo.pDynamicState = &dynamicStateCi;
        }
        void UpdateAllArrayAddresses() {
            createInfo.pStages = shaderStages.data();
            vertexInputStateCi.pVertexBindingDescriptions = vertexInputBindings.data();
            vertexInputStateCi.pVertexAttributeDescriptions = vertexInputAttributes.data();
            viewportStateCi.pViewports = viewports.data();
            viewportStateCi.pScissors = scissors.data();
            colorBlendStateCi.pAttachments = colorBlendAttachmentStates.data();
            dynamicStateCi.pDynamicStates = dynamicStates.data();
        }
    };
    class deviceMemory {
        VkDeviceMemory handle = VK_NULL_HANDLE;
        VkDeviceSize allocationSize = 0; //实际分配的内存大小
        VkMemoryPropertyFlags memoryProperties = 0; //内存属性
        //--------------------
        //该函数用于在映射内存区时，调整非host coherent的内存区域的范围
        VkDeviceSize AdjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const {
            const VkDeviceSize& nonCoherentAtomSize = graphicsBase::Base().PhysicalDeviceProperties().limits.nonCoherentAtomSize;
            VkDeviceSize _offset = offset;
            offset = offset / nonCoherentAtomSize * nonCoherentAtomSize;
            size = std::min((size + _offset + nonCoherentAtomSize - 1) / nonCoherentAtomSize * nonCoherentAtomSize, allocationSize) - offset;
            return _offset - offset;
        }
    protected:
        //用于bufferMemory或imageMemory，定义于此以节省8个字节
        class {
            friend class bufferMemory;
            friend class imageMemory;
            bool value = false;
            operator bool() const { return value; }
            auto& operator=(bool value) { this->value = value; return *this; }
        } areBound;
    public:
        deviceMemory() = default;
        deviceMemory(VkMemoryAllocateInfo& allocateInfo) {
            Allocate(allocateInfo);
        }
        deviceMemory(deviceMemory&& other) noexcept {
            MoveHandle;
            allocationSize = other.allocationSize;
            memoryProperties = other.memoryProperties;
            other.allocationSize = 0;
            other.memoryProperties = 0;
        }
        ~deviceMemory() { DestroyHandleBy(vkFreeMemory); allocationSize = 0; memoryProperties = 0; }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        VkDeviceSize AllocationSize() const { return allocationSize; }
        VkMemoryPropertyFlags MemoryProperties() const { return memoryProperties; }
        //Const Function
        //映射host visible的内存区
        result_t MapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const {
            VkDeviceSize inverseDeltaOffset;
            if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
                inverseDeltaOffset = AdjustNonCoherentMemoryRange(size, offset);
            if (VkResult result = vkMapMemory(graphicsBase::Base().Device(), handle, offset, size, 0, &pData)) {
                outStream << std::format("[ deviceMemory ] ERROR\nFailed to map the memory!\nError code: {}\n", int32_t(result));
                return result;
            }
            if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                pData = static_cast<uint8_t*>(pData) + inverseDeltaOffset;
                VkMappedMemoryRange mappedMemoryRange = {
                        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                        .memory = handle,
                        .offset = offset,
                        .size = size
                };
                if (VkResult result = vkInvalidateMappedMemoryRanges(graphicsBase::Base().Device(), 1, &mappedMemoryRange)) {
                    outStream << std::format("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
                    return result;
                }
            }
            return VK_SUCCESS;
        }
        //取消映射host visible的内存区
        result_t UnmapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const {
            if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                AdjustNonCoherentMemoryRange(size, offset);
                VkMappedMemoryRange mappedMemoryRange = {
                        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                        .memory = handle,
                        .offset = offset,
                        .size = size
                };
                if (VkResult result = vkFlushMappedMemoryRanges(graphicsBase::Base().Device(), 1, &mappedMemoryRange)) {
                    outStream << std::format("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
                    return result;
                }
            }
            vkUnmapMemory(graphicsBase::Base().Device(), handle);
            return VK_SUCCESS;
        }
        //BufferData(...)用于方便地更新设备内存区，适用于用memcpy(...)向内存区写入数据后立刻取消映射的情况
        result_t BufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
            void* pData_dst;
            if (VkResult result = MapMemory(pData_dst, size, offset))
                return result;
            memcpy(pData_dst, pData_src, size_t(size));
            return UnmapMemory(size, offset);
        }
        result_t BufferData(const auto& data_src) const {
            return BufferData(&data_src, sizeof data_src);
        }
        //RetrieveData(...)用于方便地从设备内存区取回数据，适用于用memcpy(...)从内存区取得数据后立刻取消映射的情况
        result_t RetrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const {
            void* pData_src;
            if (VkResult result = MapMemory(pData_src, size, offset))
                return result;
            memcpy(pData_dst, pData_src, size_t(size));
            return UnmapMemory(size, offset);
        }
        //Non-const Function
        result_t Allocate(VkMemoryAllocateInfo& allocateInfo) {
            if (allocateInfo.memoryTypeIndex >= graphicsBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount) {
                outStream << std::format("[ deviceMemory ] ERROR\nInvalid memory type index!\n");
                return VK_RESULT_MAX_ENUM; //没有合适的错误代码，别用VK_ERROR_UNKNOWN
            }
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            if (VkResult result = vkAllocateMemory(graphicsBase::Base().Device(), &allocateInfo, nullptr, &handle)) {
                outStream << std::format("[ deviceMemory ] ERROR\nFailed to allocate memory!\nError code: {}\n", int32_t(result));
                return result;
            }
            //记录实际分配的内存大小
            allocationSize = allocateInfo.allocationSize;
            //取得内存属性
            memoryProperties = graphicsBase::Base().PhysicalDeviceMemoryProperties().memoryTypes[allocateInfo.memoryTypeIndex].propertyFlags;
            return VK_SUCCESS;
        }
    };
    class buffer {
        VkBuffer handle = VK_NULL_HANDLE;
    public:
        buffer() = default;
        buffer(VkBufferCreateInfo& createInfo) {
            Create(createInfo);
        }
        buffer(buffer&& other) noexcept { MoveHandle; }
        ~buffer() { DestroyHandleBy(vkDestroyBuffer); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Const Function
        VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
            VkMemoryAllocateInfo memoryAllocateInfo = {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
            };
            VkMemoryRequirements memoryRequirements;
            vkGetBufferMemoryRequirements(graphicsBase::Base().Device(), handle, &memoryRequirements);
            memoryAllocateInfo.allocationSize = memoryRequirements.size;
            memoryAllocateInfo.memoryTypeIndex = UINT32_MAX;
            auto& physicalDeviceMemoryProperties = graphicsBase::Base().PhysicalDeviceMemoryProperties();
            for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
                if (memoryRequirements.memoryTypeBits & 1 << i &&
                    (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties) {
                    memoryAllocateInfo.memoryTypeIndex = i;
                    break;
                }
            //不在此检查是否成功取得内存类型索引，因为会把memoryAllocateInfo返回出去，交由外部检查
            //if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX)
            //    outStream << std::format("[ buffer ] ERROR\nFailed to find any memory type satisfies all desired memory properties!\n");
            return memoryAllocateInfo;
        }
        result_t BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const {
            VkResult result = vkBindBufferMemory(graphicsBase::Base().Device(), handle, deviceMemory, memoryOffset);
            if (result)
                outStream << std::format("[ buffer ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
            return result;
        }
        //Non-const Function
        result_t Create(VkBufferCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            VkResult result = vkCreateBuffer(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ buffer ] ERROR\nFailed to create a buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
    };
    class bufferMemory :buffer, deviceMemory {
    public:
        bufferMemory() = default;
        bufferMemory(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
            Create(createInfo, desiredMemoryProperties);
        }
        bufferMemory(bufferMemory&& other) noexcept :
                buffer(std::move(other)), deviceMemory(std::move(other)) {
            areBound = other.areBound;
            other.areBound = false;
        }
        ~bufferMemory() { areBound = false; }
        //Getter
        //不定义到VkBuffer和VkDeviceMemory的转换函数，因为32位下这俩类型都是uint64_t的别名，会造成冲突（虽然，谁他妈还用32位PC！）
        VkBuffer Buffer() const { return static_cast<const buffer&>(*this); }
        const VkBuffer* AddressOfBuffer() const { return buffer::Address(); }
        VkDeviceMemory Memory() const { return static_cast<const deviceMemory&>(*this); }
        const VkDeviceMemory* AddressOfMemory() const { return deviceMemory::Address(); }
        //若areBond为true，则成功分配了设备内存、创建了缓冲区，且成功绑定在一起
        bool AreBound() const { return areBound; }
        using deviceMemory::AllocationSize;
        using deviceMemory::MemoryProperties;
        //Const Function
        using deviceMemory::MapMemory;
        using deviceMemory::UnmapMemory;
        using deviceMemory::BufferData;
        using deviceMemory::RetrieveData;
        //Non-const Function
        //以下三个函数仅用于Create(...)可能执行失败的情况
        result_t CreateBuffer(VkBufferCreateInfo& createInfo) {
            return buffer::Create(createInfo);
        }
        result_t AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties) {
            VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
            if (allocateInfo.memoryTypeIndex >= graphicsBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount)
                return VK_RESULT_MAX_ENUM; //没有合适的错误代码，别用VK_ERROR_UNKNOWN
            return Allocate(allocateInfo);
        }
        result_t BindMemory() {
            if (VkResult result = buffer::BindMemory(Memory()))
                return result;
            areBound = true;
            return VK_SUCCESS;
        }
        //分配设备内存、创建缓冲、绑定
        result_t Create(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
            VkResult result;
            false || //这行用来应对Visual Studio中代码的对齐
            (result = CreateBuffer(createInfo)) || //用||短路执行
            (result = AllocateMemory(desiredMemoryProperties)) ||
            (result = BindMemory());
            return result;
        }
    };
    class bufferView {
        VkBufferView handle = VK_NULL_HANDLE;
    public:
        bufferView() = default;
        bufferView(VkBufferViewCreateInfo& createInfo) {
            Create(createInfo);
        }
        bufferView(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*VkBufferViewCreateFlags flags*/) {
            Create(buffer, format, offset, range);
        }
        bufferView(bufferView&& other) noexcept { MoveHandle; }
        ~bufferView() { DestroyHandleBy(vkDestroyBufferView); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Non-const Function
        result_t Create(VkBufferViewCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            VkResult result = vkCreateBufferView(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ bufferView ] ERROR\nFailed to create a buffer view!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t Create(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*VkBufferViewCreateFlags flags*/) {
            VkBufferViewCreateInfo createInfo = {
                    .buffer = buffer,
                    .format = format,
                    .offset = offset,
                    .range = range
            };
            return Create(createInfo);
        }
    };
    class image {
        VkImage handle = VK_NULL_HANDLE;
    public:
        image() = default;
        image(VkImageCreateInfo& createInfo) {
            Create(createInfo);
        }
        image(image&& other) noexcept { MoveHandle; }
        ~image() { DestroyHandleBy(vkDestroyImage); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Const Function
        VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
            VkMemoryAllocateInfo memoryAllocateInfo = {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
            };
            VkMemoryRequirements memoryRequirements;
            vkGetImageMemoryRequirements(graphicsBase::Base().Device(), handle, &memoryRequirements);
            memoryAllocateInfo.allocationSize = memoryRequirements.size;
            auto GetMemoryTypeIndex = [](uint32_t memoryTypeBits, VkMemoryPropertyFlags desiredMemoryProperties) {
                auto& physicalDeviceMemoryProperties = graphicsBase::Base().PhysicalDeviceMemoryProperties();
                for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
                    if (memoryTypeBits & 1 << i &&
                        (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties)
                        return i;
                return size_t(UINT32_MAX);
            };
            memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties);
            if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX &&
                desiredMemoryProperties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
                memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties & ~VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
            //不在此检查是否成功取得内存类型索引，因为会把memoryAllocateInfo返回出去，交由外部检查
            //if (memoryAllocateInfo.memoryTypeIndex == -1)
            //    outStream << std::format("[ image ] ERROR\nFailed to find any memory type satisfies all desired memory properties!\n");
            return memoryAllocateInfo;
        }
        result_t BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const {
            VkResult result = vkBindImageMemory(graphicsBase::Base().Device(), handle, deviceMemory, memoryOffset);
            if (result)
                outStream << std::format("[ image ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
            return result;
        }
        //Non-const Function
        result_t Create(VkImageCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            VkResult result = vkCreateImage(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ image ] ERROR\nFailed to create an image!\nError code: {}\n", int32_t(result));
            return result;
        }
    };
    class imageView {
        VkImageView handle = VK_NULL_HANDLE;
    public:
        imageView() = default;
        imageView(VkImageViewCreateInfo& createInfo) {
            Create(createInfo);
        }
        imageView(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0) {
            Create(image, viewType, format, subresourceRange, flags);
        }
        imageView(imageView&& other) noexcept { MoveHandle; }
        ~imageView() { DestroyHandleBy(vkDestroyImageView); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Non-const Function
        result_t Create(VkImageViewCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            VkResult result = vkCreateImageView(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ imageView ] ERROR\nFailed to create an image view!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t Create(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0) {
            VkImageViewCreateInfo createInfo = {
                    .flags = flags,
                    .image = image,
                    .viewType = viewType,
                    .format = format,
                    .subresourceRange = subresourceRange
            };
            return Create(createInfo);
        }
    };
    class imageMemory :image, deviceMemory {
    public:
        imageMemory() = default;
        imageMemory(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
            Create(createInfo, desiredMemoryProperties);
        }
        imageMemory(imageMemory&& other) noexcept :
                image(std::move(other)), deviceMemory(std::move(other)) {
            areBound = other.areBound;
            other.areBound = false;
        }
        ~imageMemory() { areBound = false; }
        //Getter
        VkImage Image() const { return static_cast<const image&>(*this); }
        const VkImage* AddressOfImage() const { return image::Address(); }
        VkDeviceMemory Memory() const { return static_cast<const deviceMemory&>(*this); }
        const VkDeviceMemory* AddressOfMemory() const { return deviceMemory::Address(); }
        bool AreBound() const { return areBound; }
        using deviceMemory::AllocationSize;
        using deviceMemory::MemoryProperties;
        //Non-const Function
        //以下三个函数仅用于Create(...)可能执行失败的情况
        result_t CreateImage(VkImageCreateInfo& createInfo) {
            return image::Create(createInfo);
        }
        result_t AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties) {
            VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
            if (allocateInfo.memoryTypeIndex >= graphicsBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount)
                return VK_RESULT_MAX_ENUM; //没有合适的错误代码，别用VK_ERROR_UNKNOWN
            return Allocate(allocateInfo);
        }
        result_t BindMemory() {
            if (VkResult result = image::BindMemory(Memory()))
                return result;
            areBound = true;
            return VK_SUCCESS;
        }
        //分配设备内存、创建图像、绑定
        result_t Create(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
            VkResult result;
            false || //这行用来应对Visual Studio中代码的对齐
            (result = CreateImage(createInfo)) || //用||短路执行
            (result = AllocateMemory(desiredMemoryProperties)) ||
            (result = BindMemory());
            return result;
        }
    };

    class graphicsBasePlus {
        VkFormatProperties formatProperties[std::size(formatInfos_v1_0)] = {};
        commandPool commandPool_graphics;
        commandPool commandPool_presentation;
        commandPool commandPool_compute;
        commandBuffer commandBuffer_transfer;
        commandBuffer commandBuffer_presentation;
        //Static
        static graphicsBasePlus singleton;
        //--------------------
        graphicsBasePlus() {
            auto Initialize = [] {
                if (graphicsBase::Base().QueueFamilyIndex_Graphics() != VK_QUEUE_FAMILY_IGNORED)
                    singleton.commandPool_graphics.Create(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
                            singleton.commandPool_graphics.AllocateBuffers(singleton.commandBuffer_transfer);
                if (graphicsBase::Base().QueueFamilyIndex_Compute() != VK_QUEUE_FAMILY_IGNORED)
                    singleton.commandPool_compute.Create(graphicsBase::Base().QueueFamilyIndex_Compute(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                if (graphicsBase::Base().QueueFamilyIndex_Presentation() != VK_QUEUE_FAMILY_IGNORED &&
                    graphicsBase::Base().QueueFamilyIndex_Presentation() != graphicsBase::Base().QueueFamilyIndex_Graphics() &&
                    graphicsBase::Base().SwapchainCreateInfo().imageSharingMode == VK_SHARING_MODE_EXCLUSIVE)
                    singleton.commandPool_presentation.Create(graphicsBase::Base().QueueFamilyIndex_Presentation(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
                            singleton.commandPool_presentation.AllocateBuffers(singleton.commandBuffer_presentation);
                for (size_t i = 0; i < std::size(singleton.formatProperties); i++)
                    vkGetPhysicalDeviceFormatProperties(graphicsBase::Base().PhysicalDevice(), VkFormat(i), &singleton.formatProperties[i]);
            };
            auto CleanUp = [] {
                singleton.commandPool_graphics.~commandPool();
                singleton.commandPool_presentation.~commandPool();
                singleton.commandPool_compute.~commandPool();
            };
            graphicsBase::Plus(singleton);
            graphicsBase::Base().AddCallback_CreateDevice(Initialize);
            graphicsBase::Base().AddCallback_DestroyDevice(CleanUp);
        }
        graphicsBasePlus(graphicsBasePlus&&) = delete;
        ~graphicsBasePlus() = default;
    public:
        //Getter
        const VkFormatProperties& FormatProperties(VkFormat format) const {
#ifndef NDEBUG
            if (uint32_t(format) >= std::size(formatInfos_v1_0))
                outStream << std::format("[ FormatProperties ] ERROR\nThis function only supports definite formats provided by VK_VERSION_1_0.\n"),
                        abort();
#endif
            return formatProperties[format];
        }
        const commandPool& CommandPool_Graphics() const { return commandPool_graphics; }
        const commandPool& CommandPool_Compute() const { return commandPool_compute; }
        const commandBuffer& CommandBuffer_Transfer() const { return commandBuffer_transfer; }
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
        result_t AcquireImageOwnership_Presentation(VkSemaphore semaphore_renderingIsOver, VkSemaphore semaphore_ownershipIsTransfered, VkFence fence = VK_NULL_HANDLE) const {
            if (VkResult result = commandBuffer_presentation.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
                return result;
            graphicsBase::Base().CmdTransferImageOwnership(commandBuffer_presentation);
            if (VkResult result = commandBuffer_presentation.End())
                return result;
            return graphicsBase::Base().SubmitCommandBuffer_Presentation(commandBuffer_presentation, semaphore_renderingIsOver, semaphore_ownershipIsTransfered, fence);
        }
    };
    inline graphicsBasePlus graphicsBasePlus::singleton;

    constexpr formatInfo FormatInfo(VkFormat format) {
#ifndef NDEBUG
        if (uint32_t(format) >= std::size(formatInfos_v1_0))
            outStream << std::format("[ FormatInfo ] ERROR\nThis function only supports definite formats provided by VK_VERSION_1_0.\n"),
                    abort();
#endif
        return formatInfos_v1_0[uint32_t(format)];
    }
    constexpr VkFormat Corresponding16BitFloatFormat(VkFormat format_32BitFloat) {
        switch (format_32BitFloat) {
            case VK_FORMAT_R32_SFLOAT:
                return VK_FORMAT_R16_SFLOAT;
            case VK_FORMAT_R32G32_SFLOAT:
                return VK_FORMAT_R16G16_SFLOAT;
            case VK_FORMAT_R32G32B32_SFLOAT:
                return VK_FORMAT_R16G16B16_SFLOAT;
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return VK_FORMAT_R16G16B16A16_SFLOAT;
        }
        return format_32BitFloat;
    }
    inline const VkFormatProperties& FormatProperties(VkFormat format) {
        return graphicsBase::Plus().FormatProperties(format);
    }

    class stagingBuffer {
        static inline class {
            stagingBuffer* pointer;

            static stagingBuffer* Create() {
                static stagingBuffer stagingBuffer;
                graphicsBase::Base().AddCallback_DestroyDevice([] { stagingBuffer.~stagingBuffer(); });
                return &stagingBuffer;
            }

        public:
            stagingBuffer& Get() const { return *pointer; }

            // 匿名类的构造函数需要使用类名声明
            decltype(auto) operator()() {
                pointer = Create();
                return *this;
            }
        } stagingBuffer_mainThread;
    protected:
        bufferMemory mbufferMemory;
        VkDeviceSize memoryUsage = 0;
        image aliasedImage;
    public:
        stagingBuffer() = default;
        stagingBuffer(VkDeviceSize size) {
            Expand(size);
        }
        //Getter
        operator VkBuffer() const { return mbufferMemory.Buffer(); }
        const VkBuffer* Address() const { return mbufferMemory.AddressOfBuffer(); }
        VkDeviceSize AllocationSize() const { return mbufferMemory.AllocationSize(); }
        VkImage AliasedImage() const { return aliasedImage; }
        //Const Function
        void RetrieveData(void* pData_src, VkDeviceSize size) const {
            mbufferMemory.RetrieveData(pData_src, size);
        }
        //Non-const Function
        void Expand(VkDeviceSize size) {
            if (size <= AllocationSize())
                return;
            Release();
            VkBufferCreateInfo bufferCreateInfo = {
                    .size = size,
                    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
            };
            mbufferMemory.Create(bufferCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        }
        void Release() {
            mbufferMemory.~bufferMemory();
        }
        void* MapMemory(VkDeviceSize size) {
            Expand(size);
            void* pData_dst = nullptr;
            mbufferMemory.MapMemory(pData_dst, size);
            memoryUsage = size;
            return pData_dst;
        }
        void UnmapMemory() {
            mbufferMemory.UnmapMemory(memoryUsage);
            memoryUsage = 0;
        }
        void BufferData(const void* pData_src, VkDeviceSize size) {
            Expand(size);
            mbufferMemory.BufferData(pData_src, size);
        }
        [[nodiscard]]
        VkImage AliasedImage2d(VkFormat format, VkExtent2D extent) {
            if (!(FormatProperties(format).linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
                return VK_NULL_HANDLE;
            VkDeviceSize imageDataSize = VkDeviceSize(FormatInfo(format).sizePerPixel) * extent.width * extent.height;
            if (imageDataSize > AllocationSize())
                return VK_NULL_HANDLE;
            VkImageFormatProperties imageFormatProperties = {};
            vkGetPhysicalDeviceImageFormatProperties(graphicsBase::Base().PhysicalDevice(),
                                                     format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0, &imageFormatProperties);
            if (extent.width > imageFormatProperties.maxExtent.width ||
                extent.height > imageFormatProperties.maxExtent.height ||
                imageDataSize > imageFormatProperties.maxResourceSize)
                return VK_NULL_HANDLE;
            VkImageCreateInfo imageCreateInfo = {
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format = format,
                    .extent = { extent.width, extent.height, 1 },
                    .mipLevels = 1,
                    .arrayLayers = 1,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .tiling = VK_IMAGE_TILING_LINEAR,
                    .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED
            };
            aliasedImage.~image();
            aliasedImage.Create(imageCreateInfo);
            VkImageSubresource subResource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
            VkSubresourceLayout subresourceLayout = {};
            vkGetImageSubresourceLayout(graphicsBase::Base().Device(), aliasedImage, &subResource, &subresourceLayout);
            if (subresourceLayout.size != imageDataSize)
                return VK_NULL_HANDLE;//No padding bytes
            aliasedImage.BindMemory(mbufferMemory.Memory());
            return aliasedImage;
        }
        //Static Function
        static VkBuffer Buffer_MainThread() {
            return stagingBuffer_mainThread.Get();
        }
        static void Expand_MainThread(VkDeviceSize size) {
            stagingBuffer_mainThread.Get().Expand(size);
        }
        static void Release_MainThread() {
            stagingBuffer_mainThread.Get().Release();
        }
        static void* MapMemory_MainThread(VkDeviceSize size) {
            return stagingBuffer_mainThread.Get().MapMemory(size);
        }
        static void UnmapMemory_MainThread() {
            stagingBuffer_mainThread.Get().UnmapMemory();
        }
        static void BufferData_MainThread(const void* pData_src, VkDeviceSize size) {
            stagingBuffer_mainThread.Get().BufferData(pData_src, size);
        }
        static void RetrieveData_MainThread(void* pData_src, VkDeviceSize size) {
            stagingBuffer_mainThread.Get().RetrieveData(pData_src, size);
        }
        [[nodiscard]]
        static VkImage AliasedImage2d_MainThread(VkFormat format, VkExtent2D extent) {
            return stagingBuffer_mainThread.Get().AliasedImage2d(format, extent);
        }
    };

    class deviceLocalBuffer {
    protected:
        bufferMemory mbufferMemory;
    public:
        deviceLocalBuffer() = default;
        deviceLocalBuffer(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst) {
            Create(size, desiredUsages_Without_transfer_dst);
        }
        void Destroy() {
            graphicsBase::Base().WaitIdle();
            mbufferMemory.~bufferMemory();
        }
        //Getter
        operator VkBuffer() const { return mbufferMemory.Buffer(); }
        const VkBuffer* Address() const { return mbufferMemory.AddressOfBuffer(); }
        VkDeviceSize AllocationSize() const { return mbufferMemory.AllocationSize(); }
        //Const Function
        void TransferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
            if (mbufferMemory.MemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                mbufferMemory.BufferData(pData_src, size, offset);
                return;
            }
            stagingBuffer::BufferData_MainThread(pData_src, size);
            auto& commandBuffer = graphicsBase::Plus().CommandBuffer_Transfer();
            commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            VkBufferCopy region = { 0, offset, size };
            vkCmdCopyBuffer(commandBuffer, stagingBuffer::Buffer_MainThread(), mbufferMemory.Buffer(), 1, &region);
            commandBuffer.End();
            graphicsBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
        }
        void TransferData(const void* pData_src, uint32_t elementCount, VkDeviceSize elementSize, VkDeviceSize stride_src, VkDeviceSize stride_dst, VkDeviceSize offset = 0) const {
            if (mbufferMemory.MemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                void* pData_dst = nullptr;
                mbufferMemory.MapMemory(pData_dst, stride_dst * elementCount, offset);
                for (size_t i = 0; i < elementCount; i++)
                    memcpy(stride_dst * i + static_cast<uint8_t*>(pData_dst), stride_src * i + static_cast<const uint8_t*>(pData_src), size_t(elementSize));
                mbufferMemory.UnmapMemory(elementCount * stride_dst, offset);
                return;
            }
            stagingBuffer::BufferData_MainThread(pData_src, stride_src * elementCount);
            auto& commandBuffer = graphicsBase::Plus().CommandBuffer_Transfer();
            commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            std::unique_ptr<VkBufferCopy[]> regions = std::make_unique<VkBufferCopy[]>(elementCount);
            for (size_t i = 0; i < elementCount; i++)
                regions[i] = { stride_src * i, stride_dst * i + offset, elementSize };
            vkCmdCopyBuffer(commandBuffer, stagingBuffer::Buffer_MainThread(), mbufferMemory.Buffer(), elementCount, regions.get());
            commandBuffer.End();
            graphicsBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
        }
        void TransferData(const auto& data_src) const {
            TransferData(&data_src, sizeof data_src);
        }
        void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const void* pData_src, VkDeviceSize size_Limited_to_65536, VkDeviceSize offset = 0) const {
            vkCmdUpdateBuffer(commandBuffer, mbufferMemory.Buffer(), offset, size_Limited_to_65536, pData_src);
        }
        void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const auto& data_src) const {
            vkCmdUpdateBuffer(commandBuffer, mbufferMemory.Buffer(), 0, sizeof data_src, &data_src);
        }
        //Non-const Function
        void Create(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst) {
            VkBufferCreateInfo bufferCreateInfo = {
                    .size = size,
                    .usage = desiredUsages_Without_transfer_dst | VK_BUFFER_USAGE_TRANSFER_DST_BIT
            };
            false ||
            mbufferMemory.CreateBuffer(bufferCreateInfo) ||
            mbufferMemory.AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            mbufferMemory.AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ||
            mbufferMemory.BindMemory();
        }
        void Recreate(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst) {
            graphicsBase::Base().WaitIdle();
            mbufferMemory.~bufferMemory();
            Create(size, desiredUsages_Without_transfer_dst);
        }
    };

    class vertexBuffer :public deviceLocalBuffer {
    public:
        vertexBuffer() = default;
        vertexBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :deviceLocalBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages) {}
        //Non-const Function
        void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
            deviceLocalBuffer::Create(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages);
        }
        void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
            deviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages);
        }
    };

    class indexBuffer :public deviceLocalBuffer {
    public:
        indexBuffer() = default;
        indexBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :deviceLocalBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsages) {}
        //Non-const Function
        void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
            deviceLocalBuffer::Create(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsages);
        }
        void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
            deviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsages);
        }
    };

    class uniformBuffer :public deviceLocalBuffer {
    public:
        uniformBuffer() = default;
        uniformBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :deviceLocalBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsages) {}
        //Non-const Function
        void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
            deviceLocalBuffer::Create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsages);
        }
        void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
            deviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsages);
        }
        //Static Function
        static VkDeviceSize CalculateAlignedSize(VkDeviceSize dataSize) {
            const VkDeviceSize& alignment = graphicsBase::Base().PhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
            return dataSize + alignment - 1 & ~(alignment - 1);
        }
    };

    class storageBuffer :public deviceLocalBuffer {
    public:
        storageBuffer() = default;
        storageBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :deviceLocalBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsages) {}
        //Non-const Function
        void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
            deviceLocalBuffer::Create(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsages);
        }
        void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
            deviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsages);
        }
        //Static Function
        static VkDeviceSize CalculateAlignedSize(VkDeviceSize dataSize) {
            const VkDeviceSize& alignment = graphicsBase::Base().PhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;
            return dataSize + alignment - 1 & ~(alignment - 1);
        }
    };

    class descriptorSetLayout {
        VkDescriptorSetLayout handle = VK_NULL_HANDLE;
    public:
        descriptorSetLayout() = default;
        descriptorSetLayout(VkDescriptorSetLayoutCreateInfo& createInfo) {
            Create(createInfo);
        }
        descriptorSetLayout(descriptorSetLayout&& other) noexcept { MoveHandle; }
        ~descriptorSetLayout() { DestroyHandleBy(vkDestroyDescriptorSetLayout); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Non-const Function
        result_t Create(VkDescriptorSetLayoutCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            VkResult result = vkCreateDescriptorSetLayout(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ descriptorSetLayout ] ERROR\nFailed to create a descriptor set layout!\nError code: {}\n", int32_t(result));
            return result;
        }
    };

    class descriptorSet {
        friend class descriptorPool;
        VkDescriptorSet handle = VK_NULL_HANDLE;
    public:
        descriptorSet() = default;
        descriptorSet(descriptorSet&& other) noexcept { MoveHandle; }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Const Function
        void Write(arrayRef<const VkDescriptorImageInfo> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const {
            VkWriteDescriptorSet writeDescriptorSet = {
                    .dstSet = handle,
                    .dstBinding = dstBinding,
                    .dstArrayElement = dstArrayElement,
                    .descriptorCount = uint32_t(descriptorInfos.Count()),
                    .descriptorType = descriptorType,
                    .pImageInfo = descriptorInfos.Pointer()
            };
            Update(writeDescriptorSet);
        }
        void Write(arrayRef<const VkDescriptorBufferInfo> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const {
            VkWriteDescriptorSet writeDescriptorSet = {
                    .dstSet = handle,
                    .dstBinding = dstBinding,
                    .dstArrayElement = dstArrayElement,
                    .descriptorCount = uint32_t(descriptorInfos.Count()),
                    .descriptorType = descriptorType,
                    .pBufferInfo = descriptorInfos.Pointer()
            };
            Update(writeDescriptorSet);
        }
        void Write(arrayRef<const VkBufferView> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const {
            VkWriteDescriptorSet writeDescriptorSet = {
                    .dstSet = handle,
                    .dstBinding = dstBinding,
                    .dstArrayElement = dstArrayElement,
                    .descriptorCount = uint32_t(descriptorInfos.Count()),
                    .descriptorType = descriptorType,
                    .pTexelBufferView = descriptorInfos.Pointer()
            };
            Update(writeDescriptorSet);
        }
        void Write(arrayRef<const bufferView> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const {
            Write({ descriptorInfos[0].Address(), descriptorInfos.Count() }, descriptorType, dstBinding, dstArrayElement);
        }
        //Static Function
        static void Update(arrayRef<VkWriteDescriptorSet> writes, arrayRef<VkCopyDescriptorSet> copies = {}) {
            for (auto& i : writes)
                i.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            for (auto& i : copies)
                i.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
            vkUpdateDescriptorSets(
                    graphicsBase::Base().Device(), writes.Count(), writes.Pointer(), copies.Count(), copies.Pointer());
        }
    };

    class descriptorPool {
        VkDescriptorPool handle = VK_NULL_HANDLE;
    public:
        descriptorPool() = default;
        descriptorPool(VkDescriptorPoolCreateInfo& createInfo) {
            Create(createInfo);
        }
        descriptorPool(uint32_t maxSetCount, arrayRef<const VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags = 0) {
            Create(maxSetCount, poolSizes, flags);
        }
        descriptorPool(descriptorPool&& other) noexcept { MoveHandle; }
        ~descriptorPool() { DestroyHandleBy(vkDestroyDescriptorPool); }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Const Function
        result_t AllocateSets(arrayRef<VkDescriptorSet> sets, arrayRef<const VkDescriptorSetLayout> setLayouts) const {
            if (sets.Count() != setLayouts.Count())
                if (sets.Count() < setLayouts.Count()) {
                    outStream << std::format("[ descriptorPool ] ERROR\nFor each descriptor set, must provide a corresponding layout!\n");
                    return VK_RESULT_MAX_ENUM;//没有合适的错误代码，别用VK_ERROR_UNKNOWN
                }
                else
                    outStream << std::format("[ descriptorPool ] WARNING\nProvided layouts are more than sets!\n");
            VkDescriptorSetAllocateInfo allocateInfo = {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                    .descriptorPool = handle,
                    .descriptorSetCount = uint32_t(sets.Count()),
                    .pSetLayouts = setLayouts.Pointer()
            };
            VkResult result = vkAllocateDescriptorSets(graphicsBase::Base().Device(), &allocateInfo, sets.Pointer());
            if (result)
                outStream << std::format("[ descriptorPool ] ERROR\nFailed to allocate descriptor sets!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t AllocateSets(arrayRef<VkDescriptorSet> sets, arrayRef<const descriptorSetLayout> setLayouts) const {
            return AllocateSets(
                    sets,
                    { setLayouts[0].Address(), setLayouts.Count() });
        }
        result_t AllocateSets(arrayRef<descriptorSet> sets, arrayRef<const VkDescriptorSetLayout> setLayouts) const {
            return AllocateSets(
                    { &sets[0].handle, sets.Count() },
                    setLayouts);
        }
        result_t AllocateSets(arrayRef<descriptorSet> sets, arrayRef<const descriptorSetLayout> setLayouts) const {
            return AllocateSets(
                    { &sets[0].handle, sets.Count() },
                    { setLayouts[0].Address(), setLayouts.Count() });
        }
        result_t FreeSets(arrayRef<VkDescriptorSet> sets) const {
            VkResult result = vkFreeDescriptorSets(graphicsBase::Base().Device(), handle, sets.Count(), sets.Pointer());
            memset(sets.Pointer(), 0, sets.Count() * sizeof(VkDescriptorSet));
            return result;//Though vkFreeDescriptorSets(...) can only return VK_SUCCESS
        }
        result_t FreeSets(arrayRef<descriptorSet> sets) const {
            return FreeSets({ &sets[0].handle, sets.Count() });
        }
        //Non-const Function
        result_t Create(VkDescriptorPoolCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            VkResult result = vkCreateDescriptorPool(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ descriptorPool ] ERROR\nFailed to create a descriptor pool!\nError code: {}\n", int32_t(result));
            return result;
        }
        result_t Create(uint32_t maxSetCount, arrayRef<const VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags = 0) {
            VkDescriptorPoolCreateInfo createInfo = {
                    .flags = flags,
                    .maxSets = maxSetCount,
                    .poolSizeCount = uint32_t(poolSizes.Count()),
                    .pPoolSizes = poolSizes.Pointer()
            };
            return Create(createInfo);
        }
    };
    class attachment {
	protected:
		imageView mimageView;
		imageMemory mimageMemory;
		//--------------------
		attachment() = default;
	public:
		//Getter
		VkImageView ImageView() const { return mimageView; }
		VkImage Image() const { return mimageMemory.Image(); }
		const VkImageView* AddressOfImageView() const { return mimageView.Address(); }
		const VkImage* AddressOfImage() const { return mimageMemory.AddressOfImage(); }
		//Const Function
		VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler) const {
			return { sampler, mimageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}
	};

	class colorAttachment :public attachment {
	public:
		colorAttachment() = default;
		colorAttachment(VkFormat format, VkExtent2D extent, uint32_t layerCount = 1, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0) {
			Create(format, extent, layerCount, sampleCount, otherUsages);
		}
		//Non-const Function
		void Create(VkFormat format, VkExtent2D extent, uint32_t layerCount = 1, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0) {
			VkImageCreateInfo imageCreateInfo = {
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = { extent.width, extent.height, 1 },
				.mipLevels = 1,
				.arrayLayers = layerCount,
				.samples = sampleCount,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | otherUsages
			};
			mimageMemory.Create(
				imageCreateInfo,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | bool(otherUsages & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) * VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
			mimageView.Create(
				mimageMemory.Image(),
				layerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
				format,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layerCount });
		}
		//Static Function
		static bool FormatAvailability(VkFormat format, bool supportBlending = true) {
			return FormatProperties(format).optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT << uint32_t(supportBlending);
		}
	};
	class depthStencilAttachment :public attachment {
	public:
		depthStencilAttachment() = default;
		depthStencilAttachment(VkFormat format, VkExtent2D extent, uint32_t layerCount = 1, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0, bool stencilOnly = false) {
			Create(format, extent, layerCount, sampleCount, otherUsages, stencilOnly);
		}
		//Non-const Function
		void Create(VkFormat format, VkExtent2D extent, uint32_t layerCount = 1, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0, bool stencilOnly = false) {
			VkImageCreateInfo imageCreateInfo = {
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = { extent.width, extent.height, 1 },
				.mipLevels = 1,
				.arrayLayers = layerCount,
				.samples = sampleCount,
				.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | otherUsages
			};
			mimageMemory.Create(
				imageCreateInfo,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | bool(otherUsages & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) * VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
			VkImageAspectFlags aspectMask = (!stencilOnly) * VK_IMAGE_ASPECT_DEPTH_BIT;
			if (format > VK_FORMAT_S8_UINT)
				aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			else if (format == VK_FORMAT_S8_UINT)
				aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
			mimageView.Create(
				mimageMemory.Image(),
				layerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
				format,
				{ aspectMask, 0, 1, 0, layerCount });
		}
		//Static Function
		static bool FormatAvailability(VkFormat format) {
			return FormatProperties(format).optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
	};

    struct imageOperation {
        struct imageMemoryBarrierParameterPack {
            const bool isNeeded = false;
            const VkPipelineStageFlags stage = 0;
            const VkAccessFlags access = 0;
            const VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
            constexpr imageMemoryBarrierParameterPack() = default;
            constexpr imageMemoryBarrierParameterPack(VkPipelineStageFlags stage, VkAccessFlags access, VkImageLayout layout) :
                    isNeeded(true), stage(stage), access(access), layout(layout) {}
        };
        static void CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, const VkBufferImageCopy& region,
                                         imageMemoryBarrierParameterPack imb_from, imageMemoryBarrierParameterPack imb_to) {
            //Pre-copy barrier
            VkImageMemoryBarrier imageMemoryBarrier = {
                    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    nullptr,
                    imb_from.access,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    imb_from.layout,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_QUEUE_FAMILY_IGNORED,//No ownership transfer
                    VK_QUEUE_FAMILY_IGNORED,
                    image,
                    {
                            region.imageSubresource.aspectMask,
                            region.imageSubresource.mipLevel,
                            1,
                            region.imageSubresource.baseArrayLayer,
                            region.imageSubresource.layerCount }
            };
            if (imb_from.isNeeded)
                vkCmdPipelineBarrier(commandBuffer, imb_from.stage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                     0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            //Copy
            vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            //Post-copy barrier
            if (imb_to.isNeeded) {
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageMemoryBarrier.dstAccessMask = imb_to.access;
                imageMemoryBarrier.newLayout = imb_to.layout;
                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, imb_to.stage, 0,
                                     0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }
        }
        static void CmdBlitImage(VkCommandBuffer commandBuffer, VkImage image_src, VkImage image_dst, const VkImageBlit& region,
                                 imageMemoryBarrierParameterPack imb_dst_from, imageMemoryBarrierParameterPack imb_dst_to, VkFilter filter = VK_FILTER_LINEAR) {
            //Pre-blit barrier
            VkImageMemoryBarrier imageMemoryBarrier = {
                    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    nullptr,
                    imb_dst_from.access,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    imb_dst_from.layout,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    image_dst,
                    {
                            region.dstSubresource.aspectMask,
                            region.dstSubresource.mipLevel,
                            1,
                            region.dstSubresource.baseArrayLayer,
                            region.dstSubresource.layerCount }
            };
            if (imb_dst_from.isNeeded)
                vkCmdPipelineBarrier(commandBuffer, imb_dst_from.stage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                     0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            //Blit
            vkCmdBlitImage(commandBuffer,
                           image_src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image_dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &region, filter);
            //Post-blit barrier
            if (imb_dst_to.isNeeded) {
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageMemoryBarrier.dstAccessMask = imb_dst_to.access;
                imageMemoryBarrier.newLayout = imb_dst_to.layout;
                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, imb_dst_to.stage, 0,
                                     0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }
        }
        static void CmdGenerateMipmap2d(VkCommandBuffer commandBuffer, VkImage image, VkExtent2D imageExtent, uint32_t mipLevelCount, uint32_t layerCount,
                                        imageMemoryBarrierParameterPack imb_to, VkFilter minFilter = VK_FILTER_LINEAR) {
            auto MipmapExtent = [](VkExtent2D imageExtent, uint32_t mipLevel) {
                VkOffset3D extent = { int32_t(imageExtent.width >> mipLevel), int32_t(imageExtent.height >> mipLevel), 1 };
                extent.x += !extent.x;
                extent.y += !extent.y;
                return extent;
            };
            //Blit
            if (layerCount > 1) {
                std::unique_ptr<VkImageBlit[]> regions = std::make_unique<VkImageBlit[]>(layerCount);
                for (uint32_t i = 1; i < mipLevelCount; i++) {
                    VkOffset3D mipmapExtent_src = MipmapExtent(imageExtent, i - 1);
                    VkOffset3D mipmapExtent_dst = MipmapExtent(imageExtent, i);
                    for (uint32_t j = 0; j < layerCount; j++)
                        regions[j] = {
                                { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, j, 1 },	//srcSubresource
                                { {}, mipmapExtent_src },					//srcOffsets
                                { VK_IMAGE_ASPECT_COLOR_BIT, i, j, 1 },		//dstSubresource
                                { {}, mipmapExtent_dst }					//dstOffsets
                        };
                    //Pre-blit barrier
                    VkImageMemoryBarrier imageMemoryBarrier = {
                            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                            nullptr,
                            0,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_QUEUE_FAMILY_IGNORED,
                            VK_QUEUE_FAMILY_IGNORED,
                            image,
                            { VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, layerCount }
                    };
                    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                         0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                    //Blit
                    vkCmdBlitImage(commandBuffer,
                                   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   layerCount, regions.get(), minFilter);
                    //Post-blit barrier
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                         0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                }
            }
            else
                for (uint32_t i = 1; i < mipLevelCount; i++) {
                    VkImageBlit region = {
                            { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, layerCount },//srcSubresource
                            { {}, MipmapExtent(imageExtent, i - 1) },			//srcOffsets
                            { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, layerCount },	//dstSubresource
                            { {}, MipmapExtent(imageExtent, i) }				//dstOffsets
                    };
                    CmdBlitImage(commandBuffer, image, image, region,
                                 { VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
                                 { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }, minFilter);
                }
            //Post-blit barrier
            if (imb_to.isNeeded) {
                VkImageMemoryBarrier imageMemoryBarrier = {
                        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        nullptr,
                        0,
                        imb_to.access,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        imb_to.layout,
                        VK_QUEUE_FAMILY_IGNORED,
                        VK_QUEUE_FAMILY_IGNORED,
                        image,
                        { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, layerCount }
                };
                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, imb_to.stage, 0,
                                     0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }
        }
    };

    class texture {
    protected:
        imageView mimageView;
        imageMemory mimageMemory;
        //--------------------
        texture() = default;
        void CreateImageMemory(VkImageType imageType, VkFormat format, VkExtent3D extent, uint32_t mipLevelCount, uint32_t arrayLayerCount, VkImageCreateFlags flags = 0) {
            VkImageCreateInfo imageCreateInfo = {
                    .flags = flags,
                    .imageType = imageType,
                    .format = format,
                    .extent = extent,
                    .mipLevels = mipLevelCount,
                    .arrayLayers = arrayLayerCount,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            };
            mimageMemory.Create(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }
        void CreateImageView(VkImageViewType viewType, VkFormat format, uint32_t mipLevelCount, uint32_t arrayLayerCount, VkImageViewCreateFlags flags = 0) {
            mimageView.Create(mimageMemory.Image(), viewType, format, { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, arrayLayerCount }, flags);
        }
        //Static Function
        static std::unique_ptr<uint8_t[]> LoadFile_Internal(const auto* address, size_t fileSize, VkExtent2D& extent, formatInfo requiredFormatInfo) {
#ifndef NDEBUG
            if (!(requiredFormatInfo.rawDataType == formatInfo::floatingPoint && requiredFormatInfo.sizePerComponent == 4) &&
                !(requiredFormatInfo.rawDataType == formatInfo::integer && Between_Closed<int32_t>(1, requiredFormatInfo.sizePerComponent, 2)))
                outStream << std::format("[ texture ] ERROR\nRequired format is not available for source image data!\n"),
                        abort();
#endif
            int& width = reinterpret_cast<int&>(extent.width);
            int& height = reinterpret_cast<int&>(extent.height);
            int channelCount;
            void* pImageData = nullptr;
            if constexpr (std::same_as<decltype(address), const char*>) {
                if (requiredFormatInfo.rawDataType == formatInfo::integer)
                    if (requiredFormatInfo.sizePerComponent == 1)
                        pImageData = stbi_load(address, &width, &height, &channelCount, requiredFormatInfo.componentCount);
                    else
                        pImageData = stbi_load_16(address, &width, &height, &channelCount, requiredFormatInfo.componentCount);
                else
                    pImageData = stbi_loadf(address, &width, &height, &channelCount, requiredFormatInfo.componentCount);
                if (!pImageData)
                    outStream << std::format("[ texture ] ERROR\nFailed to load the file: {}\n", address);
            }
            if constexpr (std::same_as<decltype(address), const uint8_t*>) {
                if (fileSize > INT32_MAX) {
                    outStream << std::format("[ texture ] ERROR\nFailed to load image data from the given address! Data size must be less than 2G!\n");
                    return {};
                }
                if (requiredFormatInfo.rawDataType == formatInfo::integer)
                    if (requiredFormatInfo.sizePerComponent == 1)
                        pImageData = stbi_load_from_memory(address, fileSize, &width, &height, &channelCount, requiredFormatInfo.componentCount);
                    else
                        pImageData = stbi_load_16_from_memory(address, fileSize, &width, &height, &channelCount, requiredFormatInfo.componentCount);
                else
                    pImageData = stbi_loadf_from_memory(address, fileSize, &width, &height, &channelCount, requiredFormatInfo.componentCount);
                if (!pImageData)
                    outStream << std::format("[ texture ] ERROR\nFailed to load image data from the given address!\n");
            }
            return std::unique_ptr<uint8_t[]>(static_cast<uint8_t*>(pImageData));
        }
    public:
        //Getter
        VkImageView ImageView() const { return mimageView; }
        VkImage Image() const { return mimageMemory.Image(); }
        const VkImageView* AddressOfImageView() const { return mimageView.Address(); }
        const VkImage* AddressOfImage() const { return mimageMemory.AddressOfImage(); }
        //Const Function
        VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler) const {
            return { sampler, mimageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        }
        //Static Function
        /*CheckArguments(...) should only be called in tests*/
        static bool CheckArguments(VkImageType imageType, VkExtent3D extent, uint32_t arrayLayerCount, VkFormat format_initial, VkFormat format_final, bool generateMipmap) {
            auto AliasedImageAvailability = [](VkImageType imageType, VkFormat format, VkExtent3D extent, uint32_t arrayLayerCount, VkImageUsageFlags usage) {
                if (!(FormatProperties(format).linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
                    return false;
                VkImageFormatProperties imageFormatProperties = {};
                vkGetPhysicalDeviceImageFormatProperties(
                        graphicsBase::Base().PhysicalDevice(),
                        format,
                        imageType,
                        VK_IMAGE_TILING_LINEAR,
                        usage,
                        0,
                        &imageFormatProperties);
                VkDeviceSize imageDataSize = VkDeviceSize(FormatInfo(format).sizePerPixel) * extent.width * extent.height * extent.depth;
                return
                        extent.width <= imageFormatProperties.maxExtent.width &&
                        extent.height <= imageFormatProperties.maxExtent.height &&
                        extent.depth <= imageFormatProperties.maxExtent.depth &&
                        arrayLayerCount <= imageFormatProperties.maxArrayLayers &&
                        imageDataSize <= imageFormatProperties.maxResourceSize;
            };
            if (FormatProperties(format_final).optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
                //Case: Copy data from pre-initialized image to final image
                if (FormatProperties(format_initial).linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)
                    if (AliasedImageAvailability(imageType, format_initial, extent, arrayLayerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT))
                        if (FormatProperties(format_final).optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT &&
                            generateMipmap * (FormatProperties(format_final).optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
                            return true;
                //Case: Copy data from staging buffer to final image
                if (format_initial == format_final)
                    return
                            FormatProperties(format_final).optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT &&
                            generateMipmap * (FormatProperties(format_final).optimalTilingFeatures & (VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT));
                    //Case: Copy data from staging buffer to initial image, then blit initial image to final image
                else
                    return
                            FormatProperties(format_initial).optimalTilingFeatures & (VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT) &&
                            FormatProperties(format_final).optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT &&
                            generateMipmap * (FormatProperties(format_final).optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
            }
            return false;
        }
        [[nodiscard]]
        static std::unique_ptr<uint8_t[]> LoadFile(const char* filepath, VkExtent2D& extent, formatInfo requiredFormatInfo) {
            return LoadFile_Internal(filepath, 0, extent, requiredFormatInfo);
        }
        [[nodiscard]]
        static std::unique_ptr<uint8_t[]> LoadFile(const uint8_t* fileBinaries, size_t fileSize, VkExtent2D& extent, formatInfo requiredFormatInfo) {
            return LoadFile_Internal(fileBinaries, fileSize, extent, requiredFormatInfo);
        }
        static uint32_t CalculateMipLevelCount(VkExtent2D extent) {
            return uint32_t(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
        }
        static void CopyBlitAndGenerateMipmap2d(VkBuffer buffer_copyFrom, VkImage image_copyTo, VkImage image_blitTo, VkExtent2D imageExtent,
                                                uint32_t mipLevelCount = 1, uint32_t layerCount = 1, VkFilter minFilter = VK_FILTER_LINEAR) {
            static constexpr imageOperation::imageMemoryBarrierParameterPack imbs[2] = {
                    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }
            };
            bool generateMipmap = mipLevelCount > 1;
            bool blitMipLevel0 = image_copyTo != image_blitTo;
            auto& commandBuffer = graphicsBase::Plus().CommandBuffer_Transfer();
            commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            VkBufferImageCopy region = {
                    .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
                    .imageExtent = { imageExtent.width, imageExtent.height, 1 }
            };
            imageOperation::CmdCopyBufferToImage(commandBuffer, buffer_copyFrom, image_copyTo, region,
                                                 { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[generateMipmap || blitMipLevel0]);
            //Blit to another image if necessary
            if (blitMipLevel0) {
                VkImageBlit region = {
                        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
                        { {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } },
                        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
                        { {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } }
                };
                imageOperation::CmdBlitImage(commandBuffer, image_copyTo, image_blitTo, region,
                                             { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[generateMipmap], minFilter);
            }
            //Generate mipmap if necessary, transition layout
            if (generateMipmap)
                imageOperation::CmdGenerateMipmap2d(commandBuffer, image_blitTo, imageExtent, mipLevelCount, layerCount,
                                                    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, minFilter);
            commandBuffer.End();
            //Submit
            graphicsBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
        }
        static void BlitAndGenerateMipmap2d(VkImage image_preinitialized, VkImage image_final, VkExtent2D imageExtent,
                                            uint32_t mipLevelCount = 1, uint32_t layerCount = 1, VkFilter minFilter = VK_FILTER_LINEAR) {
            static constexpr imageOperation::imageMemoryBarrierParameterPack imbs[2] = {
                    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }
            };
            bool generateMipmap = mipLevelCount > 1;
            bool blitMipLevel0 = image_preinitialized != image_final;
            if (generateMipmap || blitMipLevel0) {
                auto& commandBuffer = graphicsBase::Plus().CommandBuffer_Transfer();
                commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
                //Blit to another image if necessary
                if (blitMipLevel0) {
                    VkImageMemoryBarrier imageMemoryBarrier = {
                            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                            nullptr,
                            0,
                            VK_ACCESS_TRANSFER_READ_BIT,
                            VK_IMAGE_LAYOUT_PREINITIALIZED,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            VK_QUEUE_FAMILY_IGNORED,
                            VK_QUEUE_FAMILY_IGNORED,
                            image_preinitialized,
                            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layerCount }
                    };
                    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                         0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                    VkImageBlit region = {
                            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
                            { {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } },
                            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
                            { {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } }
                    };
                    imageOperation::CmdBlitImage(commandBuffer, image_preinitialized, image_final, region,
                                                 { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[generateMipmap], minFilter);
                }
                //Generate mipmap if necessary, transition layout
                if (generateMipmap)
                    imageOperation::CmdGenerateMipmap2d(commandBuffer, image_final, imageExtent, mipLevelCount, layerCount,
                                                        { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, minFilter);
                commandBuffer.End();
                //Submit
                graphicsBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
            }
        }
        static VkSamplerCreateInfo SamplerCreateInfo() {
            return {
                    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                    .magFilter = VK_FILTER_LINEAR,
                    .minFilter = VK_FILTER_LINEAR,
                    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    .mipLodBias = 0.f,
                    .anisotropyEnable = VK_TRUE,
                    .maxAnisotropy = graphicsBase::Base().PhysicalDeviceProperties().limits.maxSamplerAnisotropy,
                    .compareEnable = VK_FALSE,
                    .compareOp = VK_COMPARE_OP_ALWAYS,
                    .minLod = 0.f,
                    .maxLod = VK_LOD_CLAMP_NONE,
                    .borderColor = {},
                    .unnormalizedCoordinates = VK_FALSE
            };
        }
    };

    class texture2d :public texture {
    protected:
        VkExtent2D extent = {};
        //--------------------
        void Create_Internal(VkFormat format_initial, VkFormat format_final, bool generateMipmap) {
            uint32_t mipLevelCount = generateMipmap ? CalculateMipLevelCount(extent) : 1;
            //Create image and allocate memory
            CreateImageMemory(VK_IMAGE_TYPE_2D, format_final, { extent.width, extent.height, 1 }, mipLevelCount, 1);
            //Create view
            CreateImageView(VK_IMAGE_VIEW_TYPE_2D, format_final, mipLevelCount, 1);
            //Copy data and generate mipmap
            if (format_initial == format_final)
                CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), mimageMemory.Image(), mimageMemory.Image(), extent, mipLevelCount, 1);
            else
            if (VkImage image_conversion = stagingBuffer::AliasedImage2d_MainThread(format_initial, extent))
                BlitAndGenerateMipmap2d(image_conversion, mimageMemory.Image(), extent, mipLevelCount, 1);
            else {
                VkImageCreateInfo imageCreateInfo = {
                        .imageType = VK_IMAGE_TYPE_2D,
                        .format = format_initial,
                        .extent = { extent.width, extent.height, 1 },
                        .mipLevels = 1,
                        .arrayLayers = 1,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                };
                vulkan::imageMemory imageMemory_conversion(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), imageMemory_conversion.Image(), mimageMemory.Image(), extent, mipLevelCount, 1);
            }
        }
    public:
        texture2d() = default;
        texture2d(const char* filepath, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
            Create(filepath, format_initial, format_final, generateMipmap);
        }
        texture2d(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
            Create(pImageData, extent, format_initial, format_final, generateMipmap);
        }
        //Getter
        VkExtent2D Extent() const { return extent; }
        uint32_t Width() const { return extent.width; }
        uint32_t Height() const { return extent.height; }
        //Non-const Function
        void Create(const char* filepath, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
            VkExtent2D extent;
            formatInfo formatInfo = FormatInfo(format_initial);
            std::unique_ptr<uint8_t[]> pImageData = LoadFile(filepath, extent, formatInfo);
            if (pImageData)
                Create(pImageData.get(), extent, format_initial, format_final, generateMipmap);
        }
        void Create(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
            this->extent = extent;
            //Copy data to staging buffer
            size_t imageDataSize = size_t(FormatInfo(format_initial).sizePerPixel) * extent.width * extent.height;
            stagingBuffer::BufferData_MainThread(pImageData, imageDataSize);
            //Create image and allocate memory, create image view, then copy data from staging buffer to image
            Create_Internal(format_initial, format_final, generateMipmap);
        }
    };
    class texture2dArray :public texture {
    protected:
        VkExtent2D extent = {};
        uint32_t layerCount = 0;
        //--------------------
        void Create_Internal(VkFormat format_initial, VkFormat format_final, bool generateMipmap) {
            //Create image and allocate memory
            uint32_t mipLevelCount = generateMipmap ? CalculateMipLevelCount(extent) : 1;
            CreateImageMemory(VK_IMAGE_TYPE_2D, format_final, { extent.width, extent.height, 1 }, mipLevelCount, layerCount);
            //Create view
            CreateImageView(VK_IMAGE_VIEW_TYPE_2D_ARRAY, format_final, mipLevelCount, layerCount);
            //Copy data and generate mipmap
            if (format_initial == format_final)
                CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), mimageMemory.Image(), mimageMemory.Image(), extent, mipLevelCount, layerCount);
            else {
                VkImageCreateInfo imageCreateInfo = {
                        .imageType = VK_IMAGE_TYPE_2D,
                        .format = format_initial,
                        .extent = { extent.width, extent.height, 1 },
                        .mipLevels = 1,
                        .arrayLayers = layerCount,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                };
                vulkan::imageMemory imageMemory_conversion(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), imageMemory_conversion.Image(), mimageMemory.Image(), extent, mipLevelCount, layerCount);
            }
        }
    public:
        texture2dArray() = default;
        texture2dArray(const char* filepath, VkExtent2D extentInTiles, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
            Create(filepath, extentInTiles, format_initial, format_final, generateMipmap);
        }
        texture2dArray(const uint8_t* pImageData, VkExtent2D fullExtent, VkExtent2D extentInTiles, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
            Create(pImageData, fullExtent, extentInTiles, format_initial, format_final, generateMipmap);
        }
        texture2dArray(arrayRef<const char* const> filepaths, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
            Create(filepaths, format_initial, format_final, generateMipmap);
        }
        texture2dArray(arrayRef<const uint8_t* const> psImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
            Create(psImageData, extent, format_initial, format_final, generateMipmap);
        }
        //Getter
        VkExtent2D Extent() const { return extent; }
        uint32_t Width() const { return extent.width; }
        uint32_t Height() const { return extent.height; }
        uint32_t LayerCount() const { return layerCount; }
        //Non-const Function
        void Create(const char* filepath, VkExtent2D extentInTiles, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
            if (extentInTiles.width * extentInTiles.height > graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers) {
                outStream << std::format(
                        "[ texture2dArray ] ERROR\nLayer count is out of limit! Must be less than: {}\nFile: {}\n",
                        graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers, filepath);
                return;
            }
            VkExtent2D fullExtent;
            formatInfo formatInfo = FormatInfo(format_initial);
            std::unique_ptr<uint8_t[]> pImageData = LoadFile(filepath, fullExtent, formatInfo);
            if (pImageData)
                if (fullExtent.width % extentInTiles.width ||
                    fullExtent.height % extentInTiles.height)
                    outStream << std::format(
                            "[ texture2dArray ] ERROR\nImage not available!\nFile: {}\nImage width should be in multiples of {}\nImage height should be in multiples of {}\n",
                            filepath, extentInTiles.width, extentInTiles.height);//fallthrough
                else
                    Create(pImageData.get(), fullExtent, extentInTiles, format_initial, format_final, generateMipmap);
        }
        void Create(const uint8_t* pImageData, VkExtent2D fullExtent, VkExtent2D extentInTiles, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
            layerCount = extentInTiles.width * extentInTiles.height;
            if (layerCount > graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers) {
                outStream << std::format(
                        "[ texture2dArray ] ERROR\nLayer count is out of limit! Must be less than: {}\n",
                        graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers);
                return;
            }
            if (fullExtent.width % extentInTiles.width ||
                fullExtent.height % extentInTiles.height) {
                outStream << std::format(
                        "[ texture2dArray ] ERROR\nImage not available!\nImage width should be in multiples of {}\nImage height should be in multiples of {}\n",
                        extentInTiles.width, extentInTiles.height);
                return;
            }
            extent.width = fullExtent.width / extentInTiles.width;
            extent.height = fullExtent.height / extentInTiles.height;
            size_t dataSizePerPixel = FormatInfo(format_initial).sizePerPixel;
            size_t imageDataSize = dataSizePerPixel * fullExtent.width * fullExtent.height;
            //Data rearrangement can also be peformed by using tiled regions in vkCmdCopyBufferToImage(...).
            if (extentInTiles.width == 1)
                stagingBuffer::BufferData_MainThread(pImageData, imageDataSize);
            else {
                uint8_t* pData_dst = static_cast<uint8_t*>(stagingBuffer::MapMemory_MainThread(imageDataSize));
                size_t dataSizePerRow = dataSizePerPixel * extent.width;
                for (size_t j = 0; j < extentInTiles.height; j++)
                    for (size_t i = 0; i < extentInTiles.width; i++)
                        for (size_t k = 0; k < extent.height; k++)
                            memcpy(
                                    pData_dst,
                                    pImageData + (i * extent.width + (k + j * extent.height) * fullExtent.width) * dataSizePerPixel,
                                    dataSizePerRow),
                                    pData_dst += dataSizePerRow;
                stagingBuffer::UnmapMemory_MainThread();
            }
            //Create image and allocate memory, create image view, then copy data from staging buffer to image
            Create_Internal(format_initial, format_final, generateMipmap);
        }
        void Create(arrayRef<const char* const> filepaths, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
            if (filepaths.Count() > graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers) {
                outStream << std::format(
                        "[ texture2dArray ] ERROR\nLayer count is out of limit! Must be less than: {}\n",
                        graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers);
                return;
            }
            formatInfo formatInfo = FormatInfo(format_initial);
            auto psImageData = std::make_unique<std::unique_ptr<uint8_t[]>[]>(filepaths.Count());
            for (size_t i = 0; i < filepaths.Count(); i++) {
                VkExtent2D extent_currentLayer;
                psImageData[i] = LoadFile(filepaths[i], extent_currentLayer, formatInfo);
                if (psImageData[i]) {
                    if (i == 0)
                        extent = extent_currentLayer;
                    if (extent.width == extent_currentLayer.width &&
                        extent.height == extent_currentLayer.height)
                        continue;
                    else
                        outStream << std::format(
                                "[ texture2dArray ] ERROR\nImage not available!\nFile: {}\nAll the images must be in same size!\n",
                                filepaths[i]);//fallthrough
                }
                return;
            }
            Create({ reinterpret_cast<const uint8_t* const*>(psImageData.get()), filepaths.Count() }, extent, format_initial, format_final, generateMipmap);
        }
        void Create(arrayRef<const uint8_t* const> psImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
            layerCount = psImageData.Count();
            if (layerCount > graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers) {
                outStream << std::format(
                        "[ texture2dArray ] ERROR\nLayer count is out of limit! Must be less than: {}\n",
                        graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers);
                return;
            }
            this->extent = extent;
            size_t dataSizePerImage = size_t(FormatInfo(format_initial).sizePerPixel) * extent.width * extent.height;
            size_t imageDataSize = dataSizePerImage * layerCount;
            uint8_t* pData_dst = static_cast<uint8_t*>(stagingBuffer::MapMemory_MainThread(imageDataSize));
            for (size_t i = 0; i < layerCount; i++)
                memcpy(pData_dst, psImageData[i], dataSizePerImage),
                        pData_dst += dataSizePerImage;
            stagingBuffer::UnmapMemory_MainThread();
            //Create image and allocate memory, create image view, then copy data from staging buffer to image
            Create_Internal(format_initial, format_final, generateMipmap);
        }
    };
    class textureCube :public texture {
    protected:
        VkExtent2D extent = {};
        //--------------------
        VkExtent2D GetExtentInTiles(const glm::uvec2*& facePositions, bool lookFromOutside, bool loadPreviousResult = false) {
            static constexpr glm::uvec2 facePositions_default[][6] = {
                    { { 2, 1 }, { 0, 1 }, { 1, 0 }, { 1, 2 }, { 1, 1 }, { 3, 1 } },
                    { { 2, 1 }, { 0, 1 }, { 1, 0 }, { 1, 2 }, { 3, 1 }, { 1, 1 } }
            };
            static VkExtent2D extentInTiles;
            if (loadPreviousResult)
                return extentInTiles;
            extentInTiles = { 1, 1 };
            if (!facePositions)
                facePositions = facePositions_default[lookFromOutside],
                        extentInTiles = { 4, 3 };
            else
                for (size_t i = 0; i < 6; i++) {
                    if (facePositions[i].x >= extentInTiles.width)
                        extentInTiles.width = facePositions[i].x + 1;
                    if (facePositions[i].y >= extentInTiles.height)
                        extentInTiles.height = facePositions[i].y + 1;
                }
            return extentInTiles;
        }
        void Create_Internal(VkFormat format_initial, VkFormat format_final, bool generateMipmap) {
            //Create image and allocate memory
            uint32_t mipLevelCount = generateMipmap ? CalculateMipLevelCount(extent) : 1;
            CreateImageMemory(VK_IMAGE_TYPE_2D, format_final, { extent.width, extent.height, 1 }, mipLevelCount, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
            //Create view
            CreateImageView(VK_IMAGE_VIEW_TYPE_CUBE, format_final, mipLevelCount, 6);
            //Copy data and generate mipmap
            if (format_initial == format_final)
                CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), mimageMemory.Image(), mimageMemory.Image(), extent, mipLevelCount, 6);
            else {
                VkImageCreateInfo imageCreateInfo = {
                        .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                        .imageType = VK_IMAGE_TYPE_2D,
                        .format = format_initial,
                        .extent = { extent.width, extent.height, 1 },
                        .mipLevels = 1,
                        .arrayLayers = 6,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                };
                vulkan::imageMemory imageMemory_conversion(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), imageMemory_conversion.Image(), mimageMemory.Image(), extent, mipLevelCount, 6);
            }
        }
    public:
        textureCube() = default;
        /*
          Order of facePositions[6], in left handed coordinate, looking from inside:
          right(+x) left(-x) top(+y) bottom(-y) front(+z) back(-z)
          Not related to NDC.
          If lookFromOutside is true, the order is the same.
          --------------------
          Default face positions, looking from inside, is:
          [      ][ top  ][      ][      ]
          [ left ][front ][right ][ back ]
          [      ][bottom][      ][      ]
          If lookFromOutside is true, front and back is swapped.
          What ever the facePositions are, make sure the image matches the looking which a cube is unwrapped as above.
        */
        textureCube(const char* filepath, const glm::uvec2 facePositions[6], VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
            Create(filepath, facePositions, format_initial, format_final, lookFromOutside, generateMipmap);
        }
        textureCube(const uint8_t* pImageData, VkExtent2D fullExtent, const glm::uvec2 facePositions[6], VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
            Create(pImageData, fullExtent, facePositions, format_initial, format_final, lookFromOutside, generateMipmap);
        }
        textureCube(const char* const* filepaths, VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
            Create(filepaths, format_initial, format_final, lookFromOutside, generateMipmap);
        }
        textureCube(const uint8_t* const* psImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
            Create(psImageData, extent, format_initial, format_final, lookFromOutside, generateMipmap);
        }
        //Getter
        VkExtent2D Extent() const { return extent; }
        uint32_t Width() const { return extent.width; }
        uint32_t Height() const { return extent.height; }
        //Non-const Function
        void Create(const char* filepath, const glm::uvec2 facePositions[6], VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
            VkExtent2D fullExtent;
            formatInfo formatInfo = FormatInfo(format_initial);
            std::unique_ptr<uint8_t[]> pImageData = LoadFile(filepath, fullExtent, formatInfo);
            if (pImageData)
                if (VkExtent2D extentInTiles = GetExtentInTiles(facePositions, lookFromOutside);
                        fullExtent.width % extentInTiles.width ||
                        fullExtent.height % extentInTiles.height)
                    outStream << std::format(
                            "[ textureCube ] ERROR\nImage not available!\nFile: {}\nImage width should be in multiples of {}\nImage height should be in multiples of {}\n",
                            filepath, extentInTiles.width, extentInTiles.height);//fallthrough
                else {
                    extent.width = fullExtent.width / extentInTiles.width;
                    extent.height = fullExtent.height / extentInTiles.height;
                    Create(pImageData.get(), { fullExtent.width, UINT32_MAX }, facePositions, format_initial, format_final, lookFromOutside, generateMipmap);
                }
        }
        void Create(const uint8_t* pImageData, VkExtent2D fullExtent, const glm::uvec2 facePositions[6], VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
            VkExtent2D extentInTiles;
            if (fullExtent.height == UINT32_MAX)//See previous Create(...), value of fullExtent.height doesn't matter after this if statement
                extentInTiles = GetExtentInTiles(facePositions, lookFromOutside, true);
            else {
                extentInTiles = GetExtentInTiles(facePositions, lookFromOutside);
                if (fullExtent.width % extentInTiles.width ||
                    fullExtent.height % extentInTiles.height) {
                    outStream << std::format(
                            "[ textureCube ] ERROR\nImage not available!\nImage width should be in multiples of {}\nImage height should be in multiples of {}\n",
                            extentInTiles.width, extentInTiles.height);
                    return;
                }
                extent.width = fullExtent.width / extentInTiles.width;
                extent.height = fullExtent.height / extentInTiles.height;
            }
            size_t dataSizePerPixel = FormatInfo(format_initial).sizePerPixel;
            size_t dataSizePerRow = dataSizePerPixel * extent.width;
            size_t dataSizePerImage = dataSizePerRow * extent.height;
            size_t imageDataSize = dataSizePerImage * 6;
            uint8_t* pData_dst = static_cast<uint8_t*>(stagingBuffer::MapMemory_MainThread(imageDataSize));
            if (lookFromOutside)
                for (size_t face = 0; face < 6; face++)
                    if (face != 2 && face != 3)
                        for (uint32_t i = 0; i < extent.height; i++)
                            for (uint32_t j = 0; j < extent.width; j++)
                                memcpy(
                                        pData_dst,
                                        pImageData + dataSizePerPixel * (extent.width - 1 - j + facePositions[face].x * extent.width + (i + facePositions[face].y * extent.height) * fullExtent.width),
                                        dataSizePerPixel),
                                        pData_dst += dataSizePerPixel;
                    else
                        for (uint32_t j = 0; j < extent.height; j++)
                            for (uint32_t k = 0; k < extent.width; k++)
                                memcpy(
                                        pData_dst,
                                        pImageData + dataSizePerPixel * (k + facePositions[face].x * extent.width + ((extent.height - 1 - j) + facePositions[face].y * extent.height) * fullExtent.width),
                                        dataSizePerPixel),
                                        pData_dst += dataSizePerPixel;
            else
            if (extentInTiles.width == 1 && extentInTiles.height == 6 &&
                facePositions[0].y == 0 && facePositions[1].y == 1 &&
                facePositions[2].y == 2 && facePositions[3].y == 3 &&
                facePositions[4].y == 4 && facePositions[5].y == 5)
                memcpy(pData_dst, pImageData, imageDataSize);
            else
                for (size_t face = 0; face < 6; face++)
                    for (uint32_t j = 0; j < extent.height; j++)
                        memcpy(
                                pData_dst,
                                pImageData + dataSizePerPixel * (facePositions[face].x * extent.width + (j + facePositions[face].y * extent.height) * fullExtent.width),
                                dataSizePerRow),
                                pData_dst += dataSizePerRow;
            stagingBuffer::UnmapMemory_MainThread();
            //Create image and allocate memory, create image view, then copy data from staging buffer to image
            Create_Internal(format_initial, format_final, generateMipmap);
        }
        void Create(const char* const* filepaths, VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
            formatInfo formatInfo = FormatInfo(format_initial);
            std::unique_ptr<uint8_t[]> psImageData[6] = {};
            for (size_t i = 0; i < 6; i++) {
                VkExtent2D extent_currentLayer;
                psImageData[i] = LoadFile(filepaths[i], extent_currentLayer, formatInfo);
                if (psImageData[i]) {
                    if (i == 0)
                        extent = extent_currentLayer;
                    if (extent.width == extent_currentLayer.width ||
                        extent.height == extent_currentLayer.height)
                        continue;
                    else
                        outStream << std::format(
                                "[ textureCube ] ERROR\nImage not available!\nFile: {}\nAll the images must be in same size!\n",
                                filepaths[i]);//fallthrough
                }
                return;
            }
            Create(reinterpret_cast<const uint8_t* const*>(psImageData), extent, format_initial, format_final, lookFromOutside, generateMipmap);
        }
        void Create(const uint8_t* const* psImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
            this->extent = extent;
            size_t dataSizePerPixel = FormatInfo(format_initial).sizePerPixel;
            size_t dataSizePerImage = dataSizePerPixel * extent.width * extent.height;
            size_t imageDataSize = dataSizePerImage * 6;
            uint8_t* pData_dst = static_cast<uint8_t*>(stagingBuffer::MapMemory_MainThread(imageDataSize));
            if (lookFromOutside)
                for (size_t face = 0; face < 6; face++)
                    if (face != 2 && face != 3)
                        for (uint32_t j = 0; j < extent.height; j++)
                            for (uint32_t i = 0; i < extent.width; i++)
                                memcpy(
                                        pData_dst,
                                        psImageData[face] + dataSizePerPixel * ((j + 1) * extent.width - 1 - i),
                                        dataSizePerPixel),
                                        pData_dst += dataSizePerPixel;
                    else
                        for (uint32_t j = 0; j < extent.height; j++)
                            for (uint32_t i = 0; i < extent.width; i++)
                                memcpy(
                                        pData_dst,
                                        psImageData[face] + dataSizePerPixel * ((extent.height - 1 - j) * extent.width + i),
                                        dataSizePerPixel),
                                        pData_dst += dataSizePerPixel;
            else
                for (size_t i = 0; i < 6; i++)
                    memcpy(pData_dst + dataSizePerImage * i, psImageData[i], dataSizePerImage);
            stagingBuffer::UnmapMemory_MainThread();
            //Create image and allocate memory, create image view, then copy data from staging buffer to image
            Create_Internal(format_initial, format_final, generateMipmap);
        }
    };
}

#endif