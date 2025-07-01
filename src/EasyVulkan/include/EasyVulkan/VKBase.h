#pragma once
#include "EasyVKStart.h"
#include "VkGraphicsBasePlus.h"
#include "VkResultT.h"
#include <new>
#include <memory>
#define VK_RESULT_THROW

#include "VkUtil.h"

#ifndef NDEBUG
#define ENABLE_DEBUG_MESSENGER true
//Prevent binding an implicitly generated rvalue to a const reference.
#ifndef DefineHandleTypeOperator
#define DefineHandleTypeOperator operator volatile decltype(handle)() const { return handle; }
#endif
#else
#define ENABLE_DEBUG_MESSENGER false
#endif

namespace vulkan {
	constexpr VkExtent2D defaultWindowSize = { 1280, 720 };

	class graphicsBasePlus;//Forward declaration

	class graphicsBase {
    protected:
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

		graphicsBasePlus pPlus;//Pimpl
//		Static
//		static graphicsBase singleton;
		//--------------------
    public:
		graphicsBase() {
            this->AddCallback_CreateDevice(std::bind(Initialize, this, std::ref(pPlus)));
			this->AddCallback_DestroyDevice(std::bind(Cleanup, this, std::ref(pPlus)));
        }

        void Initialize(graphicsBasePlus &gbp) {
            if (this->QueueFamilyIndex_Graphics() != VK_QUEUE_FAMILY_IGNORED) {
                gbp.commandPool_graphics->Create(this->QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                gbp.commandPool_graphics->AllocateBuffers(*gbp.commandBuffer_transfer);
            }
            if (this->QueueFamilyIndex_Compute() != VK_QUEUE_FAMILY_IGNORED)
                gbp.commandPool_compute->Create(this->QueueFamilyIndex_Compute(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            if (this->QueueFamilyIndex_Presentation() != VK_QUEUE_FAMILY_IGNORED &&
                this->QueueFamilyIndex_Presentation() != this->QueueFamilyIndex_Graphics() &&
                this->SwapchainCreateInfo().imageSharingMode == VK_SHARING_MODE_EXCLUSIVE) {
                gbp.commandPool_presentation->Create(this->QueueFamilyIndex_Presentation(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                gbp.commandPool_presentation->AllocateBuffers(*gbp.commandBuffer_presentation);
            }
            for (size_t i = 0; i < std::size(gbp.formatProperties); i++)
                vkGetPhysicalDeviceFormatProperties(this->PhysicalDevice(), VkFormat(i), &gbp.formatProperties[i]);
        }

        void Cleanup(graphicsBasePlus &gbp) {
            gbp.commandPool_graphics->~commandPool();
            gbp.commandPool_presentation->~commandPool();
            gbp.commandPool_compute->~commandPool();
        }
		graphicsBase(graphicsBase&&) = delete;
        ~graphicsBase() {
            if (!instance)
                return;
            if (device) {
                WaitIdle();
                if (swapchain) {
                    for (auto& i : callbacks_destroySwapchain)
                        i(), WaitIdle();
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
		void cleanup() {
            if (swapchain) {
                for (auto &i: callbacks_destroySwapchain)
                    i(), WaitIdle();
                for (auto &i: swapchainImageViews)
                    if (i)
                        vkDestroyImageView(device, i, nullptr);
                vkDestroySwapchainKHR(device, swapchain, nullptr);
                swapchain = VK_NULL_HANDLE;
                callbacks_createSwapchain.clear();
                callbacks_destroySwapchain.clear();
                swapchainImageViews.clear();
            }
            if (surface) {
                vkDestroySurfaceKHR(instance, surface, nullptr);
                surface = VK_NULL_HANDLE;
            }
        }
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
        VkQueue getGraphicsQueue() const {
            return queue_graphics;
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
            physicalDeviceFeatures.fillModeNonSolid = VK_TRUE;
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
			for (auto& i : callbacks_createDevice)
				i();
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
//		static graphicsBase& Base() {
//			return singleton;
//		}
//		static graphicsBasePlus& Plus() { return *singleton.pPlus; }
//		static void Plus(graphicsBasePlus& plus) { if (!singleton.pPlus) singleton.pPlus = &plus; }
	};
//	inline graphicsBase graphicsBase::singleton;

	class semaphore {
		VkSemaphore handle = VK_NULL_HANDLE;
        VkDevice *mDevice = nullptr; 
	public:
		//semaphore() = default;
		explicit semaphore(VkDevice* device, VkSemaphoreCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VkResult result = vkCreateSemaphore(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ semaphore ] ERROR\nFailed to create a semaphore!\nError code: {}\n", int32_t(result));
		}
		explicit semaphore(VkDevice *device ) : semaphore(device, {}){}
		semaphore(semaphore&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~semaphore() { if (handle) { vkDestroySemaphore(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; }}
		//Getter
        operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
		//Non-const Function
	};
	class fence {
		VkFence handle = VK_NULL_HANDLE;
        VkDevice* mDevice = nullptr;
    public:
		//fence() = default;
		fence(VkDevice* device, VkFenceCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            VkResult result = vkCreateFence(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ fence ] ERROR\nFailed to create a fence!\nError code: {}\n", int32_t(result));
		}
		fence(VkDevice* device, VkFenceCreateFlags flags = 0): fence(device, {.flags=flags}) {}
		fence(fence&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~fence() { if (handle) { vkDestroyFence(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
		//Const Function
		result_t Wait() const {
			VkResult result = vkWaitForFences(*mDevice, 1, &handle, false, UINT64_MAX);
			if (result)
				outStream << std::format("[ fence ] ERROR\nFailed to wait for the fence!\nError code: {}\n", int32_t(result));
			return result;
		}
		result_t Reset() const {
			VkResult result = vkResetFences(*mDevice, 1, &handle);
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
			VkResult result = vkGetFenceStatus(*mDevice, handle);
			if (result < 0)
				outStream << std::format("[ fence ] ERROR\nFailed to get the status of the fence!\nError code: {}\n", int32_t(result));
			return result;
		}
	};
	class event {
		VkEvent handle = VK_NULL_HANDLE;
        VkDevice* mDevice = nullptr;
    public:
		//event() = default;
		event(VkDevice* device, VkEventCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
            VkResult result = vkCreateEvent(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ event ] ERROR\nFailed to create a event!\nError code: {}\n", int32_t(result));
		}
		event(VkDevice* device, VkEventCreateFlags flags = 0) : event(device, {.flags = flags}) {}
		event(event& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~event() { if (handle) { vkDestroyEvent(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
		//Const Function
		void CmdSet(VkCommandBuffer commandBuffer, VkPipelineStageFlags stage_from) const {
			vkCmdSetEvent(commandBuffer, handle, stage_from);
		}
		void CmdReset(VkCommandBuffer commandBuffer, VkPipelineStageFlags stage_from) const {
			vkCmdResetEvent(commandBuffer, handle, stage_from);
		}
		void CmdWait(VkCommandBuffer commandBuffer, VkPipelineStageFlags stage_from, VkPipelineStageFlags stage_to,
			arrayRef<VkMemoryBarrier> memoryBarriers,
			arrayRef<VkBufferMemoryBarrier> bufferMemoryBarriers,
			arrayRef<VkImageMemoryBarrier> imageMemoryBarriers) const {
			for (auto& i : memoryBarriers)
				i.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			for (auto& i : bufferMemoryBarriers)
				i.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			for (auto& i : imageMemoryBarriers)
				i.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			vkCmdWaitEvents(commandBuffer, 1, &handle, stage_from, stage_to,
				memoryBarriers.Count(), memoryBarriers.Pointer(),
				bufferMemoryBarriers.Count(), bufferMemoryBarriers.Pointer(),
				imageMemoryBarriers.Count(), imageMemoryBarriers.Pointer());
		}
		result_t Set() const {
			VkResult result = vkSetEvent(*mDevice, handle);
			if (result)
				outStream << std::format("[ event ] ERROR\nFailed to singal the event!\nError code: {}\n", int32_t(result));
			return result;
		}
		result_t Reset() const {
			VkResult result = vkResetEvent(*mDevice, handle);
			if (result)
				outStream << std::format("[ event ] ERROR\nFailed to unsingal the event!\nError code: {}\n", int32_t(result));
			return result;
		}
		result_t Status() const {
			VkResult result = vkGetEventStatus(*mDevice, handle);
			if (result < 0)
				outStream << std::format("[ event ] ERROR\nFailed to get the status of the event!\nError code: {}\n", int32_t(result));
			return result;
		}
	};

	class deviceMemory {
    protected:
		VkDeviceMemory handle = VK_NULL_HANDLE;
		VkDeviceSize allocationSize = 0;
		VkMemoryPropertyFlags memoryProperties = 0;
        VkDevice* mDevice = nullptr;
        VkPhysicalDeviceProperties* mPhysicalDeviceProperties = nullptr;
        VkPhysicalDeviceMemoryProperties* mPhysicalDeviceMemoryProperties = nullptr;
        
		//--------------------
		VkDeviceSize AdjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const {
			const VkDeviceSize& nonCoherentAtomSize = (*mPhysicalDeviceProperties).limits.nonCoherentAtomSize;
			VkDeviceSize _offset = offset;
			offset = offset / nonCoherentAtomSize * nonCoherentAtomSize;
			size = std::min((size + _offset + nonCoherentAtomSize - 1) / nonCoherentAtomSize * nonCoherentAtomSize, allocationSize) - offset;
			return _offset - offset;
		}
	protected:
		class {
			friend class bufferMemory;
			friend class imageMemory;
			bool value = false;
			operator bool() const { return value; }
			auto& operator=(bool value) { this->value = value; return *this; }
		} areBound;
	public:
		deviceMemory(VkDevice* device,
                     VkPhysicalDeviceProperties *physicalDeviceProperties,
                     VkPhysicalDeviceMemoryProperties *physicalDeviceMemoryProperties) : deviceMemory(std::move(device),
                                                                                                      std::move(physicalDeviceProperties),
                                                                                                      std::move(physicalDeviceMemoryProperties),
                                                                                                      VkMemoryAllocateInfo{ .allocationSize = 0, .memoryTypeIndex = 0 }) {}
		deviceMemory(VkDevice* device, 
                     VkPhysicalDeviceProperties *physicalDeviceProperties,
                     VkPhysicalDeviceMemoryProperties *physicalDeviceMemoryProperties,
                     VkMemoryAllocateInfo&& allocateInfo): mDevice(std::move(device)),
                                                           mPhysicalDeviceProperties(std::move(physicalDeviceProperties)),
                                                           mPhysicalDeviceMemoryProperties(std::move(physicalDeviceMemoryProperties)) {
            if(Allocate(allocateInfo) != VK_SUCCESS)
                throw std::runtime_error("[ deviceMemory ] ERROR\nFailed to allocate memory!");
		}
		deviceMemory(deviceMemory&& other) noexcept {
            mDevice = std::move(other.mDevice);
            mPhysicalDeviceProperties = std::move(other.mPhysicalDeviceProperties);
            mPhysicalDeviceMemoryProperties = std::move(other.mPhysicalDeviceMemoryProperties);
			handle = other.handle;
            other.handle = VK_NULL_HANDLE;
			allocationSize = other.allocationSize;
			memoryProperties = other.memoryProperties;
			other.allocationSize = 0;
			other.memoryProperties = 0;
		}
		~deviceMemory() {
            if (handle) {
                vkFreeMemory(*mDevice, handle, nullptr);
                handle = VK_NULL_HANDLE;
            }
            allocationSize = 0;
            memoryProperties = 0;
        }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
		VkDeviceSize AllocationSize() const { return allocationSize; }
		VkMemoryPropertyFlags MemoryProperties() const { return memoryProperties; }
		//Const Function
		result_t MapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const {
			VkDeviceSize inverseDeltaOffset;
			if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
				inverseDeltaOffset = AdjustNonCoherentMemoryRange(size, offset);
			if (VkResult result = vkMapMemory(*mDevice, handle, offset, size, 0, &pData)) {
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
				if (VkResult result = vkInvalidateMappedMemoryRanges(*mDevice, 1, &mappedMemoryRange)) {
					outStream << std::format("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
					return result;
				}
			}
			return VK_SUCCESS;
		}
		result_t UnmapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const {
			if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				AdjustNonCoherentMemoryRange(size, offset);
				VkMappedMemoryRange mappedMemoryRange = {
					.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
					.memory = handle,
					.offset = offset,
					.size = size
				};
				if (VkResult result = vkFlushMappedMemoryRanges(*mDevice, 1, &mappedMemoryRange)) {
					outStream << std::format("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
					return result;
				}
			}
			vkUnmapMemory(*mDevice, handle);
			return VK_SUCCESS;
		}
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
		result_t RetrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const {
			void* pData_src;
			if (VkResult result = MapMemory(pData_src, size, offset))
				return result;
			memcpy(pData_dst, pData_src, size_t(size));
			return UnmapMemory(size, offset);
		}
        result_t Allocate(VkMemoryAllocateInfo& allocateInfo) {
            if (allocateInfo.memoryTypeIndex >= mPhysicalDeviceMemoryProperties->memoryTypeCount) {
                outStream << std::format("[ deviceMemory ] ERROR\nInvalid memory type index!\n");
                return VK_RESULT_MAX_ENUM;
            }
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            if (VkResult result = vkAllocateMemory(*mDevice, &allocateInfo, nullptr, &handle)) {
                outStream << std::format("[ deviceMemory ] ERROR\nFailed to allocate memory!\nError code: {}\n", int32_t(result));
                return result;
            }
            allocationSize = allocateInfo.allocationSize;
            memoryProperties = mPhysicalDeviceMemoryProperties->memoryTypes[allocateInfo.memoryTypeIndex].propertyFlags;
            return VK_SUCCESS;
        }
	};
	class buffer {
    protected:
        VkBuffer handle = VK_NULL_HANDLE;
        VkDevice *mDevice = nullptr;
        VkPhysicalDeviceMemoryProperties *mPhysicalDeviceMemoryProperties = nullptr;
	public:
		buffer(VkDevice* device,
               VkPhysicalDeviceMemoryProperties *physicalDeviceMemoryProperties) : buffer(std::move(device), std::move(physicalDeviceMemoryProperties), {}){}
		buffer(VkDevice* device,
               VkPhysicalDeviceMemoryProperties *physicalDeviceMemoryProperties,
               VkBufferCreateInfo&& createInfo) : mDevice(std::move(device)), mPhysicalDeviceMemoryProperties(std::move(physicalDeviceMemoryProperties)) {
            if (Create(std::move(createInfo)) != VK_SUCCESS)
                throw std::runtime_error("[ buffer ] ERROR\nFailed to create a buffer!");
        }
		buffer(buffer&& other) noexcept {
            handle = other.handle;
            other.handle = VK_NULL_HANDLE;
            mDevice = std::move(other.mDevice);
            mPhysicalDeviceMemoryProperties = std::move(other.mPhysicalDeviceMemoryProperties);
        }
		~buffer() { if (handle) { vkDestroyBuffer(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
		//Const Function
		VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
			VkMemoryAllocateInfo memoryAllocateInfo = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
			};
			VkMemoryRequirements memoryRequirements;
			vkGetBufferMemoryRequirements(*mDevice, handle, &memoryRequirements);
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			memoryAllocateInfo.memoryTypeIndex = UINT32_MAX;
			auto& physicalDeviceMemoryProperties = *mPhysicalDeviceMemoryProperties;
			for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
				if (memoryRequirements.memoryTypeBits & 1 << i &&
					(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties) {
					memoryAllocateInfo.memoryTypeIndex = i;
					break;
				}
			return memoryAllocateInfo;
		}
		result_t BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const {
			VkResult result = vkBindBufferMemory(*mDevice, handle, deviceMemory, memoryOffset);
			if (result)
				outStream << std::format("[ buffer ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
			return result;
		}
        result_t Create(VkBufferCreateInfo&& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            VkResult result = vkCreateBuffer(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ buffer ] ERROR\nFailed to create a buffer!\nError code: {}\n", int32_t(result));
            return result;
        }
	};
	class bufferMemory :buffer, deviceMemory {
	public:
		bufferMemory(VkDevice *device,
                     VkPhysicalDeviceProperties *physicalDeviceProperties,
                     VkPhysicalDeviceMemoryProperties *physicalDeviceMemoryProperties
        ) : buffer(device, physicalDeviceMemoryProperties), deviceMemory(device, physicalDeviceProperties, physicalDeviceMemoryProperties) {}
		bufferMemory(VkDevice *device,
                     VkPhysicalDeviceProperties *physicalDeviceProperties,
                     VkPhysicalDeviceMemoryProperties *physicalDeviceMemoryProperties,
                     VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties
        ) : buffer(device, physicalDeviceMemoryProperties), deviceMemory(device, physicalDeviceProperties, physicalDeviceMemoryProperties){

        }
		bufferMemory(bufferMemory&& other) noexcept :
			buffer(std::move(other)), deviceMemory(std::move(other)) {
			areBound = other.areBound;
			other.areBound = false;
		}
		~bufferMemory() { areBound = false; }
		//Getter
		VkBuffer Buffer() const { return static_cast<const buffer&>(*this); }
		const VkBuffer* AddressOfBuffer() const { return buffer::Address(); }
		VkDeviceMemory Memory() const { return static_cast<const deviceMemory&>(*this); }
		const VkDeviceMemory* AddressOfMemory() const { return deviceMemory::Address(); }
		bool AreBound() const { return areBound; }
		using deviceMemory::AllocationSize;
		using deviceMemory::MemoryProperties;
		//Const Function
		using deviceMemory::MapMemory;
		using deviceMemory::UnmapMemory;
		using deviceMemory::BufferData;
		using deviceMemory::RetrieveData;
		//Non-const Function
		result_t CreateBuffer(VkBufferCreateInfo&& createInfo) {
			return buffer::Create(std::move(createInfo));
		}
		result_t AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties) {
			VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
			if (allocateInfo.memoryTypeIndex >= deviceMemory::mPhysicalDeviceMemoryProperties->memoryTypeCount)
				return VK_RESULT_MAX_ENUM;
			return deviceMemory::Allocate(allocateInfo);
		}
		result_t BindMemory() {
			if (VkResult result = buffer::BindMemory(Memory()))
				return result;
			areBound = true;
			return VK_SUCCESS;
		}
		result_t Create(VkBufferCreateInfo&& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
			VkResult result;
			false ||
				(result = CreateBuffer(std::move(createInfo))) ||
				(result = AllocateMemory(desiredMemoryProperties)) ||
				(result = BindMemory());
			return result;
		}
	};
	class image {
    protected:
		VkImage handle = VK_NULL_HANDLE;
        VkDevice* mDevice = nullptr;
        VkPhysicalDeviceMemoryProperties *mPhysicalDeviceMemoryProperties = nullptr;
	public:
		image(VkDevice *device,
              VkPhysicalDeviceMemoryProperties *physicalDeviceMemoryProperties) : image(std::move(device), std::move(physicalDeviceMemoryProperties), {}) {}
		image(VkDevice *device,
              VkPhysicalDeviceMemoryProperties *physicalDeviceMemoryProperties,
              VkImageCreateInfo&& createInfo) : mDevice(std::move(device)), mPhysicalDeviceMemoryProperties(std::move(physicalDeviceMemoryProperties)) {
            if(Create(std::move(createInfo)) != VK_SUCCESS)
                throw std::runtime_error("[ image ] ERROR\nFailed to create an image!");
        }
		image(image&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); mPhysicalDeviceMemoryProperties = std::move(other.mPhysicalDeviceMemoryProperties); }
		~image() { if (handle) { vkDestroyImage(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; }}
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
		//Const Function
		VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
			VkMemoryAllocateInfo memoryAllocateInfo = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
			};
			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(*mDevice, handle, &memoryRequirements);
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			auto GetMemoryTypeIndex = [this](uint32_t memoryTypeBits, VkMemoryPropertyFlags desiredMemoryProperties) {
				auto& physicalDeviceMemoryProperties = *mPhysicalDeviceMemoryProperties;
				for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
					if (memoryTypeBits & 1 << i &&
						(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties)
						return i;
				return UINT32_MAX;
			};
			memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties);
			if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX &&
				desiredMemoryProperties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
				memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties & ~VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
			return memoryAllocateInfo;
		}
		result_t BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const {
			VkResult result = vkBindImageMemory(*mDevice, handle, deviceMemory, memoryOffset);
			if (result)
				outStream << std::format("[ image ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
			return result;
		}
        result_t Create(VkImageCreateInfo&& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            VkResult result = vkCreateImage(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ image ] ERROR\nFailed to create an image!\nError code: {}\n", int32_t(result));
            return result;
        }
	};
	class imageMemory :image, deviceMemory {
	public:
		imageMemory(
                    VkDevice* device,
                    VkPhysicalDeviceProperties *physicalDeviceProperties,
                    VkPhysicalDeviceMemoryProperties *physicalDeviceMemoryProperties
        ) : image(device, physicalDeviceMemoryProperties), deviceMemory(device, physicalDeviceProperties, physicalDeviceMemoryProperties) {}
		imageMemory(
                    VkDevice* device,
                    VkPhysicalDeviceProperties *physicalDeviceProperties,
                    VkPhysicalDeviceMemoryProperties *physicalDeviceMemoryProperties,
                    VkImageCreateInfo&& createInfo,
                    VkMemoryPropertyFlags desiredMemoryProperties
        ) : image(device, physicalDeviceMemoryProperties), deviceMemory(device, physicalDeviceProperties, physicalDeviceMemoryProperties) {
			Create(std::move(createInfo), desiredMemoryProperties);
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
		result_t CreateImage(VkImageCreateInfo&& createInfo) {
			return image::Create(std::move(createInfo));
		}
		result_t AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties) {
			VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
			if (allocateInfo.memoryTypeIndex >= deviceMemory::mPhysicalDeviceMemoryProperties->memoryTypeCount)
				return VK_RESULT_MAX_ENUM;
			return deviceMemory::Allocate(allocateInfo);
		}
		result_t BindMemory() {
			if (VkResult result = image::BindMemory(Memory()))
				return result;
			areBound = true;
			return VK_SUCCESS;
		}
		result_t Create(VkImageCreateInfo&& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
			VkResult result;
			false ||
				(result = CreateImage(std::move(createInfo))) ||
				(result = AllocateMemory(desiredMemoryProperties)) ||
				(result = BindMemory());
			return result;
		}
	};

	class bufferView {
		VkBufferView handle = VK_NULL_HANDLE;
        VkDevice* mDevice = nullptr;
	public:
		bufferView(VkDevice *device) : bufferView(device, {}) {}
        bufferView(VkDevice *device,
                   VkBuffer buffer,
                   VkFormat format,
                   VkDeviceSize offset = 0,
                   VkDeviceSize range = 0
                           /*reserved for future use*/) : bufferView(std::move(device), {.buffer = buffer, .format = format, .offset = offset, .range = range}) {}
		bufferView(VkDevice *device, VkBufferViewCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            VkResult result = vkCreateBufferView(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ bufferView ] ERROR\nFailed to create a buffer view!\nError code: {}\n", int32_t(result));
        }
		bufferView(bufferView&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~bufferView() { if (handle) { vkDestroyBufferView(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
	};
	class imageView {
		VkImageView handle = VK_NULL_HANDLE;
        VkDevice* mDevice = nullptr;
	public:
		imageView(VkDevice *device) : imageView(device, {}) {}
        imageView(VkDevice *device,
                  VkImage image,
                  VkImageViewType viewType,
                  VkFormat format,
                  const VkImageSubresourceRange& subresourceRange,
                  VkImageViewCreateFlags flags = 0) : imageView(std::move(device), {.image = image, .viewType = viewType, .format = format, .subresourceRange = subresourceRange, .flags = flags}) {}
		imageView(VkDevice *device, VkImageViewCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            VkResult result = vkCreateImageView(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ imageView ] ERROR\nFailed to create an image view!\nError code: {}\n", int32_t(result));
		}

		imageView(imageView&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~imageView() {vkDestroyImageView(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
	};
	class sampler {
		VkSampler handle = VK_NULL_HANDLE;
        VkDevice *mDevice = nullptr;
	public:
		sampler(VkDevice *device) : sampler(std::move(device), {}) {}
		sampler(VkDevice *device, VkSamplerCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            VkResult result = vkCreateSampler(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ sampler ] ERROR\nFailed to create a sampler!\nError code: {}\n", int32_t(result));
		}
		sampler(sampler&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~sampler() { if (handle) { vkDestroySampler(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
	};

	class shaderModule {
		VkShaderModule handle = VK_NULL_HANDLE;
        VkDevice *mDevice;
	public:
		shaderModule(VkDevice *device) : shaderModule(std::move(device), VkShaderModuleCreateInfo{}) {}
        shaderModule(VkDevice *device, size_t codeSize, const uint32_t* pCode) : shaderModule (std::move(device), { .codeSize = codeSize, .pCode = pCode}){}
        shaderModule(VkDevice *device, const char* filepath) {
            std::ifstream file(filepath, std::ios::ate | std::ios::binary);
            if (!file) {
                outStream << std::format("[ shader ] ERROR\nFailed to open the file: {}\n", filepath);
            }
            size_t fileSize = size_t(file.tellg());
            std::vector<uint32_t> binaries(fileSize / 4);
            file.seekg(0);
            file.read(reinterpret_cast<char*>(binaries.data()), fileSize);
            file.close();
            shaderModule(std::move(device), VkShaderModuleCreateInfo{ .codeSize = fileSize, .pCode = binaries.data() });
        }
        shaderModule(VkDevice *device, VkShaderModuleCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            VkResult result = vkCreateShaderModule(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ shader ] ERROR\nFailed to create a shader module!\nError code: {}\n", int32_t(result));
        }


		shaderModule(shaderModule&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~shaderModule() { if (handle) { vkDestroyShaderModule(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; }}
        void cleanup() { if (handle) { vkDestroyShaderModule(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
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
	};
	class descriptorSetLayout {
		VkDescriptorSetLayout handle = VK_NULL_HANDLE;
        VkDevice* mDevice = nullptr;
	public:
		descriptorSetLayout(VkDevice *device) : descriptorSetLayout(std::move(device), {}) {}
		descriptorSetLayout(VkDevice *device, VkDescriptorSetLayoutCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            VkResult result = vkCreateDescriptorSetLayout(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ descriptorSetLayout ] ERROR\nFailed to create a descriptor set layout!\nError code: {}\n", int32_t(result));
        }
		descriptorSetLayout(descriptorSetLayout&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~descriptorSetLayout() { if (handle) { vkDestroyDescriptorSetLayout(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
        void cleanup() { if (handle) { vkDestroyDescriptorSetLayout(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
	};
    class pipelineLayout {
        VkPipelineLayout handle = VK_NULL_HANDLE;
        VkDevice* mDevice = nullptr;
    public:
        pipelineLayout(VkDevice *device) : pipelineLayout(std::move(device), {}) {}
        pipelineLayout(VkDevice *device, VkPipelineLayoutCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            VkResult result = vkCreatePipelineLayout(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ pipelineLayout ] ERROR\nFailed to create a pipeline layout!\nError code: {}\n", int32_t(result));
        }
        pipelineLayout(pipelineLayout&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
        ~pipelineLayout() { if (handle) { vkDestroyPipelineLayout(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
        void cleanup() { if (handle) { vkDestroyPipelineLayout(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
        //Getter
        operator decltype(handle)() const { return handle; };
        const decltype(handle)* Address() const { return &handle; };
    };
	class pipeline {
		VkPipeline handle = VK_NULL_HANDLE;
        VkDevice *mDevice = nullptr;
	public:
		pipeline(VkDevice *device) : pipeline(std::move(device), VkGraphicsPipelineCreateInfo{}) {};
		pipeline(VkDevice *device, VkGraphicsPipelineCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            VkResult result = vkCreateGraphicsPipelines(*mDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ pipeline ] ERROR\nFailed to create a graphics pipeline!\nError code: {}\n", int32_t(result));
        }
		pipeline(VkDevice *device, VkComputePipelineCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            VkResult result = vkCreateComputePipelines(*mDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ pipeline ] ERROR\nFailed to create a compute pipeline!\nError code: {}\n", int32_t(result));
		}
		pipeline(pipeline&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~pipeline() { if (handle) { vkDestroyPipeline(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
	};

	class renderPass {
		VkRenderPass handle = VK_NULL_HANDLE;
        VkDevice *mDevice = nullptr;
	public:
		renderPass(VkDevice *device) : renderPass(std::move(device), VkRenderPassCreateInfo{}) {}
		renderPass(VkDevice *device, VkRenderPassCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            VkResult result = vkCreateRenderPass(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ renderPass ] ERROR\nFailed to create a render pass!\nError code: {}\n", int32_t(result));
        }
		renderPass(renderPass&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~renderPass() { if (handle) { vkDestroyRenderPass(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
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
	};
	class framebuffer {
		VkFramebuffer handle = VK_NULL_HANDLE;
        VkDevice *mDevice = nullptr;
	public:
		framebuffer(VkDevice *device) : framebuffer(std::move(device), VkFramebufferCreateInfo{}) {}
		framebuffer(VkDevice *device, VkFramebufferCreateInfo&& createInfo) : mDevice (std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            VkResult result = vkCreateFramebuffer(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ framebuffer ] ERROR\nFailed to create a framebuffer!\nError code: {}\n", int32_t(result));
        }
		framebuffer(framebuffer&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~framebuffer() { if (handle) { vkDestroyFramebuffer(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
	};

	class descriptorSet {
		friend class descriptorPool;
		VkDescriptorSet handle = VK_NULL_HANDLE;
        VkDevice *mDevice = nullptr;
	public:
		descriptorSet(VkDevice *device) : mDevice(std::move(device)) {}
		descriptorSet(descriptorSet&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
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
			Update(*mDevice, writeDescriptorSet);
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
			Update(*mDevice, writeDescriptorSet);
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
			Update(*mDevice, writeDescriptorSet);
		}
		void Write(arrayRef<const bufferView> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const {
			Write({ descriptorInfos[0].Address(), descriptorInfos.Count() }, descriptorType, dstBinding, dstArrayElement);
		}
		//Static Function
		static void Update(VkDevice device, arrayRef<VkWriteDescriptorSet> writes, arrayRef<VkCopyDescriptorSet> copies = {}) {
			for (auto& i : writes)
				i.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			for (auto& i : copies)
				i.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			vkUpdateDescriptorSets(
				device, writes.Count(), writes.Pointer(), copies.Count(), copies.Pointer());
		}
	};
	class descriptorPool {
		VkDescriptorPool handle = VK_NULL_HANDLE;
        VkDevice *mDevice = nullptr;
	public:
		descriptorPool(VkDevice *device) : descriptorPool(std::move(device), VkDescriptorPoolCreateInfo{}) {}
		descriptorPool(VkDevice *device, VkDescriptorPoolCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            VkResult result = vkCreateDescriptorPool(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ descriptorPool ] ERROR\nFailed to create a descriptor pool!\nError code: {}\n", int32_t(result));
        }
		descriptorPool(VkDevice *device,
                       uint32_t maxSetCount,
                       arrayRef<const VkDescriptorPoolSize> poolSizes,
                       VkDescriptorPoolCreateFlags flags = 0) : descriptorPool(std::move(device),
                                                                               VkDescriptorPoolCreateInfo{.flags = flags,
                                                                                                          .maxSets = maxSetCount,
                                                                                                          .poolSizeCount = uint32_t(poolSizes.Count()),
                                                                                                          .pPoolSizes = poolSizes.Pointer()}) {}
		descriptorPool(descriptorPool&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~descriptorPool() { if (handle) { vkDestroyDescriptorPool(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
		//Const Function
		result_t AllocateSets(arrayRef<VkDescriptorSet> sets, arrayRef<const VkDescriptorSetLayout> setLayouts) const {
			if (sets.Count() != setLayouts.Count())
				if (sets.Count() < setLayouts.Count()) {
					outStream << std::format("[ descriptorPool ] ERROR\nFor each descriptor set, must provide a corresponding layout!\n");
					return VK_RESULT_MAX_ENUM;
				}
				else
					outStream << std::format("[ descriptorPool ] WARNING\nProvided layouts are more than sets!\n");
			VkDescriptorSetAllocateInfo allocateInfo = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = handle,
				.descriptorSetCount = uint32_t(sets.Count()),
				.pSetLayouts = setLayouts.Pointer()
			};
			VkResult result = vkAllocateDescriptorSets(*mDevice, &allocateInfo, sets.Pointer());
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
			VkResult result = vkFreeDescriptorSets(*mDevice, handle, sets.Count(), sets.Pointer());
			memset(sets.Pointer(), 0, sets.Count() * sizeof(VkDescriptorSet));
			return result;//Though vkFreeDescriptorSets(...) can only return VK_SUCCESS
		}
		result_t FreeSets(arrayRef<descriptorSet> sets) const {
			return FreeSets({ &sets[0].handle, sets.Count() });
		}
	};

	class queryPool {
		VkQueryPool handle = VK_NULL_HANDLE;
        VkDevice *mDevice = nullptr;
	public:
		queryPool(VkDevice *device) : queryPool(std::move(device), VkQueryPoolCreateInfo{}) {}
		queryPool(VkDevice *device, VkQueryPoolCreateInfo&& createInfo) : mDevice(std::move(device)) {
            createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            VkResult result = vkCreateQueryPool(*mDevice, &createInfo, nullptr, &handle);
            if (result)
                outStream << std::format("[ queryPool ] ERROR\nFailed to create a query pool!\nError code: {}\n", int32_t(result));
        }
		queryPool(VkDevice *device,
                  VkQueryType queryType,
                  uint32_t queryCount,
                  VkQueryPipelineStatisticFlags pipelineStatistics = 0) : queryPool(std::move(device),
                                                                                    VkQueryPoolCreateInfo{.queryType = queryType,
                                                                                                          .queryCount = queryCount,
                                                                                                          .pipelineStatistics = pipelineStatistics}){}
		queryPool(queryPool&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; mDevice = std::move(other.mDevice); }
		~queryPool() { if (handle) { vkDestroyQueryPool(*mDevice, handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		operator decltype(handle)() const { return handle; };
		const decltype(handle)* Address() const { return &handle; };
		//Const Function
		void CmdReset(VkCommandBuffer commandBuffer, uint32_t firstQueryIndex, uint32_t queryCount) const {
			vkCmdResetQueryPool(commandBuffer, handle, firstQueryIndex, queryCount);
		}
		void CmdBegin(VkCommandBuffer commandBuffer, uint32_t queryIndex, VkQueryControlFlags flags = 0) const {
			vkCmdBeginQuery(commandBuffer, handle, queryIndex, flags);
		}
		void CmdEnd(VkCommandBuffer commandBuffer, uint32_t queryIndex) const {
			vkCmdEndQuery(commandBuffer, handle, queryIndex);
		}
		void CmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, uint32_t queryIndex) const {
			vkCmdWriteTimestamp(commandBuffer, pipelineStage, handle, queryIndex);
		}
		void CmdCopyResults(VkCommandBuffer commandBuffer, uint32_t firstQueryIndex, uint32_t queryCount,
			VkBuffer buffer_dst, VkDeviceSize offset_dst, VkDeviceSize stride, VkQueryResultFlags flags = 0) const {
			vkCmdCopyQueryPoolResults(commandBuffer, handle, firstQueryIndex, queryCount, buffer_dst, offset_dst, stride, flags);
		}
		result_t GetResults(uint32_t firstQueryIndex, uint32_t queryCount, size_t dataSize, void* pData_dst, VkDeviceSize stride, VkQueryResultFlags flags = 0) const {
			VkResult result = vkGetQueryPoolResults(*mDevice, handle, firstQueryIndex, queryCount, dataSize, pData_dst, stride, flags);
			if (result)
				result > 0 ?
				outStream << std::format("[ queryPool ] WARNING\nNot all queries are available!\nError code: {}\n", int32_t(result)) :
				outStream << std::format("[ queryPool ] ERROR\nFailed to get query pool results!\nError code: {}\n", int32_t(result));
			return result;
		}
		/*Provided by VK_API_VERSION_1_2*/
		void Reset(uint32_t firstQueryIndex, uint32_t queryCount) {
			vkResetQueryPool(*mDevice, handle, firstQueryIndex, queryCount);
		}
	};
}