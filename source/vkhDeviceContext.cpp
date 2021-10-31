#include "vkhDeviceContext.hpp"
#include <set>
#include "ice.hpp"

static auto findQueue(
	std::vector<vk::QueueFamilyProperties> const& queueFamilies,
	std::string const& name,
	VkQueueFlags requiredBits,
	VkQueueFlags excludedBits)
{
	const auto family = std::find_if(queueFamilies.begin(), queueFamilies.end(), [requiredBits, excludedBits](const VkQueueFamilyProperties& queueFamily)
		{
			return
				queueFamily.queueCount > 0 &&
				queueFamily.queueFlags & requiredBits &&
				!(queueFamily.queueFlags & excludedBits);
		});

	if (family == queueFamilies.end())
		throw std::runtime_error("found no matching " + name + " queue");

	return family;
}

void vkh::DeviceContext::checkRequiredExtensions(vk::PhysicalDevice physicalDevice) const
{
	auto const availableExtensions = physicalDevice.enumerateDeviceExtensionProperties(nullptr);
	std::set<std::string> requiredExtensionsSet(requiredExtensions.begin(), requiredExtensions.end());

	for (const auto& extension : availableExtensions)
		requiredExtensionsSet.erase(extension.extensionName);

	if (!requiredExtensionsSet.empty())
	{
		bool first = true;
		std::string extensions;

		for (const auto& extension : requiredExtensionsSet)
		{
			if (!first)
				extensions += ", ";

			extensions += extension;
			first = false;
		}

		throw std::runtime_error("missing required extensions: " + extensions);
	}
}

static uint rateDeviceSuitability(vk::PhysicalDevice physicalDevice)
{
	auto const queueFamiliyProperties = physicalDevice.getQueueFamilyProperties();
	auto const properties = physicalDevice.getProperties();
	auto const features = physicalDevice.getFeatures();

	// Application can't function without geometry shaders
	if (!features.geometryShader)
		return 0;
	
	uint score = 0;
	// Discrete GPUs have a significant performance advantage
	if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		score += 1000;
	else if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
		score += 300;
	
	score += properties.limits.maxImageDimension2D;
	score += properties.limits.maxBoundDescriptorSets * 20;
	score += properties.limits.maxDescriptorSetUniformBuffers * 20;
	score += properties.limits.maxPerStageDescriptorSampledImages * 20;

	return score;
}

static vk::PhysicalDevice getSuitableDevice(vk::Instance instance) 
{
	auto physicalDevices = instance.enumeratePhysicalDevices();
	std::sort(physicalDevices.begin(), physicalDevices.end(), [](auto& a, auto& b)
		{
			return rateDeviceSuitability(a) > rateDeviceSuitability(b);
		});
	
	auto const result = std::find_if(physicalDevices.begin(), physicalDevices.end(), [](vk::PhysicalDevice const& device)
		{
			// We want a device with a graphics queue.
			auto const queueFamilies = device.getQueueFamilyProperties();

			auto const hasGraphicsQueue = std::find_if(queueFamilies.begin(), queueFamilies.end(), [](const vk::QueueFamilyProperties& queueFamily)
				{
					return queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics;
				});

			return hasGraphicsQueue != queueFamilies.end();
		});
	
	if (result == physicalDevices.end())
		throw std::runtime_error("cannot find a suitable device !");

	return *result;
}

void vkh::DeviceContext::create(vkh::Instance const& instance, vk::SurfaceKHR const& surface,
	std::span<const char*> requiredExtensions_)
{
	allocationCallbacks = instance.allocationCallbacks;
	requiredExtensions = std::vector(requiredExtensions_.begin(), requiredExtensions_.end());

	physicalDevice = getSuitableDevice(*instance.handle);

	checkRequiredExtensions(physicalDevice);

	auto const queueFamilies = physicalDevice.getQueueFamilyProperties();

	// Find the graphics queue.
	auto const graphicsFamily = findQueue(queueFamilies, "graphics", VK_QUEUE_GRAPHICS_BIT, 0);
	auto const computeFamily = findQueue(queueFamilies, "compute", VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT);
	auto const transferFamily = findQueue(queueFamilies, "transfer", VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

	// Find the presentation queue (usually the same as graphics queue).
	auto const presentFamily = std::find_if(queueFamilies.begin(), queueFamilies.end(), [&](const vk::QueueFamilyProperties& queueFamily)
		{
			VkBool32 presentSupport = false;
			uint32 const i = static_cast<uint32>(&*queueFamilies.cbegin() - &queueFamily);
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
			return queueFamily.queueCount > 0 && presentSupport;
		});

	if (presentFamily == queueFamilies.end())
		throw std::runtime_error("found no presentation queue");

	graphicsFamilyIndex = static_cast<uint32>(graphicsFamily - queueFamilies.begin());
	computeFamilyIndex = static_cast<uint32>(computeFamily - queueFamilies.begin());
	presentFamilyIndex = static_cast<uint32>(presentFamily - queueFamilies.begin());
	transferFamilyIndex = static_cast<uint32>(transferFamily - queueFamilies.begin());

	// Queues can be the same
	const std::set<uint32> uniqueQueueFamilies =
	{
		graphicsFamilyIndex,
		computeFamilyIndex,
		presentFamilyIndex,
		transferFamilyIndex
	};

	// Create queues
	float queuePriority = 1.0f;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

	for (uint32 queueFamilyIndex : uniqueQueueFamilies)
	{
		vk::DeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	vk::PhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = true;
	deviceFeatures.samplerAnisotropy = true;

	vk::PhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures = {};
	indexingFeatures.runtimeDescriptorArray = true;

	vk::DeviceCreateInfo createInfo = {};
	createInfo.pNext = &indexingFeatures;
	createInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledLayerCount = static_cast<uint32>(instance.validationLayers.size());
	createInfo.ppEnabledLayerNames = instance.validationLayers.data();
	createInfo.enabledExtensionCount = static_cast<uint32>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	device = physicalDevice.createDevice(createInfo, allocationCallbacks);

	graphicsQueue = device.getQueue(graphicsFamilyIndex, 0);
	computeQueue = device.getQueue(computeFamilyIndex, 0);
	presentQueue = device.getQueue(presentFamilyIndex, 0);
	transferQueue = device.getQueue(transferFamilyIndex, 0);

	vma::AllocatorCreateInfo allocatorInfo;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
	allocatorInfo.instance = *instance.handle;
	allocatorInfo.device = device;
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.pAllocationCallbacks = allocationCallbacks;

	gpuAllocator = vma::createAllocator(allocatorInfo);

	vk::CommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.queueFamilyIndex = graphicsFamilyIndex;
	poolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	
	commandPool = device.createCommandPool(poolCreateInfo, allocationCallbacks);
}

void vkh::DeviceContext::destroy()
{
	device.destroyCommandPool(commandPool, allocationCallbacks);
	gpuAllocator.destroy();
	device.destroy(allocationCallbacks);
}
