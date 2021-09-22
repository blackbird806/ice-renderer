#include "vkhInstance.hpp"
#include <cstdio>
#include <GLFW/glfw3.h>

#include "ice.hpp"

// @Review
static VkBool32 debugVkCallback(VkDebugUtilsMessageSeverityFlagBitsEXT  messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	VkDebugUtilsMessengerCallbackDataEXT const* callbackData,
	void* userData)
{
	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		return VK_FALSE;
	
	auto const messageTypeStr = vk::to_string(vk::DebugUtilsMessageTypeFlagsEXT(messageType));
	auto const messageSeverityStr = vk::to_string(vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity));
	printf("[vulkan debug callback (%s %s) ] : %s\n", messageTypeStr.c_str(), messageSeverityStr.c_str(), callbackData->pMessage);
	return VK_FALSE;
}

bool vkh::Instance::checkValidationLayerSupport() const
{
	std::vector<vk::LayerProperties> availablesLayers = vk::enumerateInstanceLayerProperties();
	for (auto const& layerName : validationLayers)
	{
		bool layerFound = false;
		for (auto const& layerProperties : availablesLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
			return false;
	}
	return true;
}

static std::vector<const char*> getGlfwRequiredExtensions(bool validationLayersEnabled)
{
	uint32 glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (validationLayersEnabled)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

void vkh::Instance::create(const char* appName, const char* engineName, std::span<const char*> validationLayers_, vk::AllocationCallbacks* alloc)

{
	// disable validation layers if none provided
	if (validationLayersEnabled() && !checkValidationLayerSupport())
		throw std::runtime_error("validation layers requested, but not available!");

	allocationCallbacks = alloc;
	validationLayers = std::vector(validationLayers_.begin(), validationLayers_.end());
	
	vk::ApplicationInfo appInfo = {};
	appInfo.pApplicationName = appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = engineName;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	vk::InstanceCreateInfo createInfo = {};
	createInfo.pApplicationInfo = &appInfo;

	auto glfwExtensions = getGlfwRequiredExtensions(validationLayersEnabled());

	createInfo.enabledExtensionCount = static_cast<uint32>(glfwExtensions.size());
	createInfo.ppEnabledExtensionNames = glfwExtensions.data();

	vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

	if (validationLayersEnabled())
	{
		createInfo.enabledLayerCount = static_cast<uint32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
		debugCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
		debugCreateInfo.pfnUserCallback = &debugVkCallback;
		debugCreateInfo.pUserData = nullptr;

		vk::ValidationFeatureEnableEXT enables[] = { vk::ValidationFeatureEnableEXT::eBestPractices };
		vk::ValidationFeaturesEXT features = {};
		features.enabledValidationFeatureCount = std::size(enables);
		features.pEnabledValidationFeatures = enables;
		debugCreateInfo.pNext = &features;
		createInfo.pNext = &debugCreateInfo; // @TODO
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	handle = vk::createInstanceUnique(createInfo, allocationCallbacks);

	physicalDevices = handle.get().enumeratePhysicalDevices();
	extensions = vk::enumerateInstanceExtensionProperties(nullptr);
}

void vkh::Instance::destroy()
{
	handle.reset();
}

bool vkh::Instance::validationLayersEnabled() const noexcept
{
	return !validationLayers.empty();
}
