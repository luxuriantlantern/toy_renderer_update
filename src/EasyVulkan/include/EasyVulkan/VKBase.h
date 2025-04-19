#pragma once
#include "EasyVKStart.h"
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

        std::vector<void(*)()> callbacks_createSwapchain;
        std::vector<void(*)()> callbacks_destroySwapchain;
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
        void AddCallback_CreateSwapchain(void(*function)()) {
            callbacks_createSwapchain.push_back(function);
        }
        void AddCallback_DestroySwapchain(void(*function)()) {
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
}