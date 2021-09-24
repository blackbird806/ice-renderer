#include "vulkanContextLegacy.hpp"

#include <iostream>
#include <set>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <tiny/tiny_obj_loader.h>

#include "utility.hpp"

static bool checkValidationLayerSupport(std::span<const char*> validationLayers)
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

static std::vector<const char*> getGlfwRequiredExtensions()
{
	uint32 glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifndef DISABLE_VALIDATION_LAYERS
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	
	return extensions;
}

static VkBool32 debugVkCallback(VkDebugUtilsMessageSeverityFlagBitsEXT  messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	VkDebugUtilsMessengerCallbackDataEXT const* callbackData,
	void* userData)
{
	std::cout << "[vulkan debug callback] :" << callbackData->pMessage << "\n\n";
	return VK_FALSE;
}

void VulkanContextLegacy::loadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	
	// TODO use obj v2 API
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "assets/viking_room.obj")) {
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32> uniqueVertices = {};

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex = {};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			if (!attrib.normals.empty() && index.normal_index >= 0)
			{
				float* np = &attrib.normals[3 * index.normal_index];
				//vertex.normal = { np[0], np[1], np[2] };
			}

			vertex.uv = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

vk::CommandBuffer VulkanContextLegacy::beginSingleTimeCommands()
{
	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	vk::CommandBuffer const commandBuffer = device.allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	commandBuffer.begin(beginInfo);
	
	return commandBuffer;
}

void VulkanContextLegacy::endSingleTimeCommands(vk::CommandBuffer cmdBuff)
{
	cmdBuff.end();
	
	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuff;

	graphicsQueue.submit(1, &submitInfo, vk::Fence{});
	graphicsQueue.waitIdle();

	device.freeCommandBuffers(commandPool, { cmdBuff });
}

void VulkanContextLegacy::copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size)
{
	vk::CommandBuffer const cmdBuff = beginSingleTimeCommands();

	vk::BufferCopy copyRegion{};
	copyRegion.size = size;

	cmdBuff.copyBuffer(src, dst, copyRegion);
	
	endSingleTimeCommands(cmdBuff);
}

void VulkanContextLegacy::copyBufferToImage(vk::Buffer src, vk::Image dst, uint32 width, uint32 height)
{
	vk::CommandBuffer const commandBuffer = beginSingleTimeCommands();

	vk::BufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = vk::Offset3D{ 0, 0, 0 };
	region.imageExtent = vk::Extent3D{
		width,
		height,
		1
	};

	commandBuffer.copyBufferToImage(src, dst, vk::ImageLayout::eTransferDstOptimal, { region });

	endSingleTimeCommands(commandBuffer);
}

static vk::SampleCountFlagBits getMaxUsableSampleCount(vk::PhysicalDevice physicalDevice)
{
	vk::PhysicalDeviceProperties const physicalDeviceProperties = physicalDevice.getProperties();

	vk::SampleCountFlags const counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

	if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
	if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
	if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
	if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
	if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
	if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

	return vk::SampleCountFlagBits::e1;
}

static bool hasStencilComponent(vk::Format format) noexcept
{
	return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

static vk::Format findSupportedFormat(vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, const vk::ImageTiling tiling, const vk::FormatFeatureFlags features)
{
	for (auto format : candidates)
	{
		vk::FormatProperties props = physicalDevice.getFormatProperties(format);;

		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}

		if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format");
}

static vk::Format findDepthFormat(vk::PhysicalDevice physicalDevice)
{
	return findSupportedFormat(
		physicalDevice,
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

void VulkanContextLegacy::transitionImageLayout(vk::Image image, vk::Format format, uint32 miplevel, vk::ImageLayout oldLayout,
                                          vk::ImageLayout newLayout)
{
	vk::CommandBuffer const commandBuffer = beginSingleTimeCommands();

	vk::ImageMemoryBarrier barrier{};
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = miplevel;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

		if (hasStencilComponent(format))
		{
			barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	}
	
	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlags();
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlags();
		barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else
	{
		throw std::runtime_error("unsupported layout transition");
	}

	commandBuffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
	
	endSingleTimeCommands(commandBuffer);
}

static uint32 findMemoryType(vk::PhysicalDevice const& physicalDevice, uint32 typeFilter, vk::MemoryPropertyFlags props)
{
	vk::PhysicalDeviceMemoryProperties const memProperties = physicalDevice.getMemoryProperties();

	for (uint32 i = 0; i != memProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & props) == props)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type");
}

void VulkanContextLegacy::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
	vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory, uint32 miplevel, vk::SampleCountFlagBits numSample)
{
	vk::ImageCreateInfo imageInfo{};
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = miplevel;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = usage;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;
	imageInfo.samples = numSample;

	image = device.createImage(imageInfo, allocator);

	vk::MemoryRequirements const memRequirements = device.getImageMemoryRequirements(image);

	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	imageMemory = device.allocateMemory(allocInfo, allocator);

	device.bindImageMemory(image, imageMemory, 0);
}


void VulkanContextLegacy::createInstance(std::span<const char*> validationLayers)
{
	vk::ApplicationInfo appInfo = {};
	appInfo.pApplicationName = "ice renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "ice engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	vk::InstanceCreateInfo createInfo = {};
	createInfo.pApplicationInfo = &appInfo;

	auto glfwExtensions = getGlfwRequiredExtensions();

	createInfo.enabledExtensionCount = static_cast<uint32>(glfwExtensions.size());
	createInfo.ppEnabledExtensionNames = glfwExtensions.data();

	vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

#ifndef DISABLE_VALIDATION_LAYERS
	{
		createInfo.enabledLayerCount = static_cast<uint32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
		debugCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
		debugCreateInfo.pfnUserCallback = &debugVkCallback;
		debugCreateInfo.pUserData = nullptr;

		vk::ValidationFeatureEnableEXT enables[] = { vk::ValidationFeatureEnableEXT::eBestPractices };
		vk::ValidationFeaturesEXT features = { };
		features.enabledValidationFeatureCount = std::size(enables);
		features.pEnabledValidationFeatures = enables;
		debugCreateInfo.pNext = &features;
		createInfo.pNext = &debugCreateInfo; // @TODO
	}
#else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}
#endif

	instance = vk::createInstance(createInfo, allocator);

	physicalDevices = instance.enumeratePhysicalDevices();
	extensions = vk::enumerateInstanceExtensionProperties(nullptr);
}

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

void VulkanContextLegacy::createDevice(std::span<const char*> requiredExtensions)
{
	// TODO choose real device
	physicalDevice = physicalDevices[0];
	msaaSamples = getMaxUsableSampleCount(physicalDevice);
	
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
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	for (uint32 queueFamilyIndex : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = true;
	deviceFeatures.samplerAnisotropy = true;

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures = {};
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	indexingFeatures.runtimeDescriptorArray = true;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &indexingFeatures;
	createInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledLayerCount = static_cast<uint32>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();
	createInfo.enabledExtensionCount = static_cast<uint32>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	device = physicalDevice.createDevice(createInfo, allocator);

	device.getQueue(graphicsFamilyIndex, 0, &graphicsQueue);
	device.getQueue(computeFamilyIndex, 0, &computeQueue);
	device.getQueue(presentFamilyIndex, 0, &presentQueue);
	device.getQueue(transferFamilyIndex, 0, &transferQueue);

	vma::AllocatorCreateInfo allocatorInfo;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
	allocatorInfo.instance = instance;
	allocatorInfo.device = device;
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.pAllocationCallbacks = allocator;
	
	mainAllocator = vma::createAllocator(allocatorInfo);
}

void VulkanContextLegacy::createSurface()
{
	VkSurfaceKHR tmpSurface;
	glfwCreateWindowSurface(instance, window, reinterpret_cast<VkAllocationCallbacks*>(allocator), &tmpSurface);
	surface = tmpSurface;
}

void VulkanContextLegacy::createSyncPrimitives()
{
	for (int i = 0; i < c_maxFramesInFlight; i++)
	{
		vk::SemaphoreCreateInfo semaphoreCreateInfo{};
		imageAvailableSemaphore[i] = device.createSemaphore(semaphoreCreateInfo, allocator);
		renderFinishedSemaphore[i] = device.createSemaphore(semaphoreCreateInfo, allocator);

		vk::FenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		inFlightFences[i] = device.createFence(fenceCreateInfo, allocator);
	}
}

void VulkanContextLegacy::createCommandPool()
{
	vk::CommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.queueFamilyIndex = graphicsFamilyIndex;
	poolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	commandPool = device.createCommandPool(poolCreateInfo, allocator);
}

static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
{
	if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
	{
		return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	}

	for (const auto& format : formats)
	{
		if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return format;
		}
	}

	throw std::runtime_error("found no suitable surface format");
}

static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& presentModes, bool vsync)
{
	// VK_PRESENT_MODE_IMMEDIATE_KHR: 
	//   Images submitted by your application are transferred to the screen right away, which may result in tearing.
	// VK_PRESENT_MODE_FIFO_KHR: 
	//   The swap chain is a queue where the display takes an image from the front of the queue when the display is
	//   refreshed and the program inserts rendered images at the back of the queue. If the queue is full then the program 
	//   has to wait. This is most similar to vertical sync as found in modern games. The moment that the display is 
	//   refreshed is known as "vertical blank".
	// VK_PRESENT_MODE_FIFO_RELAXED_KHR:
	//   This mode only differs from the previous one if the application is late and the queue was empty at the last 
	//   vertical blank. Instead of waiting for the next vertical blank, the image is transferred right away when it 
	//   finally arrives. This may result in visible tearing.
	// VK_PRESENT_MODE_MAILBOX_KHR: 
	//   This is another variation of the second mode. Instead of blocking the application when the queue is full, the 
	//   images that are already queued are simply replaced with the newer ones.This mode can be used to implement triple 
	//   buffering, which allows you to avoid tearing with significantly less latency issues than standard vertical sync 
	//   that uses double buffering.

	if (vsync)
	{
		return vk::PresentModeKHR::eFifo;
	}

	if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eMailbox) != presentModes.end())
	{
		return vk::PresentModeKHR::eMailbox;
	}

	if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eImmediate) != presentModes.end())
	{
		return vk::PresentModeKHR::eImmediate;
	}

	if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eFifoRelaxed) != presentModes.end())
	{
		return vk::PresentModeKHR::eFifoRelaxed;
	}

	return vk::PresentModeKHR::eFifo;
}

static auto querySwapchainSupport(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
{
	struct SupportDetails
	{
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};
	
	SupportDetails const details{
		.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface),
		.formats = physicalDevice.getSurfaceFormatsKHR(surface),
		.presentModes = physicalDevice.getSurfacePresentModesKHR(surface)
	};
	return details;
}

static vk::Extent2D chooseSwapExtent(GLFWwindow* window, const vk::SurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32>::max())
		return capabilities.currentExtent;

	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	vk::Extent2D actualExtent = { static_cast<uint32>(w), static_cast<uint32>(h) };

	actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
	actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

	return actualExtent;
}

void VulkanContextLegacy::createSwapchain()
{
	auto const details = querySwapchainSupport(physicalDevice, surface);

	if (details.formats.empty() || details.presentModes.empty())
		throw std::runtime_error("empty swap chain support");

	auto const surfaceFormat = chooseSwapSurfaceFormat(details.formats);
	auto const presentMode = chooseSwapPresentMode(details.presentModes, vsync);
	swapchainExtent = chooseSwapExtent(window, details.capabilities);

	vk::SwapchainCreateInfoKHR createInfo = {};
	createInfo.surface = surface;
	createInfo.minImageCount = c_maxFramesInFlight;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapchainExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
	createInfo.preTransform = details.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = vk::PresentModeKHR::eFifo;
	createInfo.clipped = VK_TRUE;

	if (graphicsFamilyIndex != presentFamilyIndex)
	{
		uint32_t queueFamilyIndices[] = { graphicsFamilyIndex, presentFamilyIndex };

		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	swapchain = device.createSwapchainKHR(createInfo, allocator);

	minImageCount = details.capabilities.minImageCount;
	swapchainFormat = surfaceFormat.format;
	swapchainImages = device.getSwapchainImagesKHR(swapchain);

	for (auto const& image : swapchainImages)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imageViewCreateInfo.format = swapchainFormat;
		imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		
		swapchainImageViews.push_back(device.createImageViewUnique(imageViewCreateInfo, allocator));
	}

}

void VulkanContextLegacy::createGraphicsPipeline()
{
	vk::VertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	auto const attributeDescriptions = Vertex::getAttributeDescriptions();
	
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(std::size(attributeDescriptions));
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	vk::Viewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchainExtent.width);
	viewport.height = static_cast<float>(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor = {};
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = swapchainExtent;

	vk::PipelineViewportStateCreateInfo viewportState = {};
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = isWireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	vk::PipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = msaaSamples;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	vk::PipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = vk::CompareOp::eLess;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne; // Optional
	colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero; // Optional
	colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero; // Optional
	colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd; // Optional

	vk::PipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = vk::LogicOp::eCopy; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// Create descriptor pool/sets.
	std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;

	vk::DescriptorSetLayoutBinding uboLayoutInfo;
	uboLayoutInfo.binding = 0;
	uboLayoutInfo.descriptorCount = 1;
	uboLayoutInfo.descriptorType = vk::DescriptorType::eUniformBuffer;
	uboLayoutInfo.stageFlags = vk::ShaderStageFlagBits::eVertex;

	layoutBindings.push_back(uboLayoutInfo);

	vk::DescriptorSetLayoutBinding textureLayoutInfo;
	textureLayoutInfo.binding = 1;
	textureLayoutInfo.descriptorCount = 1;
	textureLayoutInfo.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	textureLayoutInfo.stageFlags = vk::ShaderStageFlagBits::eFragment;

	layoutBindings.push_back(textureLayoutInfo);

	vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutInfo.pBindings = layoutBindings.data();

	descriptorSetLayout = device.createDescriptorSetLayout(layoutInfo, allocator);

	// Create pipeline layout and render pass.

	vk::DescriptorSetLayout descriptorSetLayouts[] = { descriptorSetLayout };

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.setLayoutCount = std::size(descriptorSetLayouts);
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo, allocator);

	// Load shaders

	// TODO use shader compiler lib
	std::system("cd .\\shaders && shadercompile.bat");

	auto vertCode = readBinFile("shaders/vert.spv");
	
	vk::ShaderModuleCreateInfo vertexShaderCreateInfo{};
	vertexShaderCreateInfo.codeSize = vertCode.size();
	vertexShaderCreateInfo.pCode = reinterpret_cast<uint32_t const*>(vertCode.data());

	vk::UniqueShaderModule vertexShader = device.createShaderModuleUnique(vertexShaderCreateInfo, allocator);

	auto fragCode = readBinFile("shaders/frag.spv");

	vk::ShaderModuleCreateInfo fragmentShaderCreateInfo{};
	fragmentShaderCreateInfo.codeSize = fragCode.size();
	fragmentShaderCreateInfo.pCode = reinterpret_cast<uint32_t const*>(fragCode.data());
	
	vk::UniqueShaderModule fragmentShader = device.createShaderModuleUnique(fragmentShaderCreateInfo, allocator);

	vk::PipelineShaderStageCreateInfo vertexStageInfo{};
	vertexStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertexStageInfo.module = vertexShader.get();
	vertexStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragmentStageInfo{};
	fragmentStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragmentStageInfo.module = fragmentShader.get();
	fragmentStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo shaderStages[] =
	{
		vertexStageInfo,
		fragmentStageInfo
	};

	vk::AttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchainFormat;
	colorAttachment.samples = msaaSamples;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eLoad;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = swapchainFormat;
	colorAttachmentResolve.samples = vk::SampleCountFlagBits::e1;
	colorAttachmentResolve.loadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachmentResolve.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachmentResolve.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachmentResolve.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachmentResolve.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachmentResolve.finalLayout = vk::ImageLayout::ePresentSrcKHR;
	
	vk::AttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat(physicalDevice);
	depthAttachment.samples = msaaSamples;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
	depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass = {};
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	vk::SubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
	dependency.srcAccessMask = vk::AccessFlagBits();
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
	dependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eColorAttachmentWrite;

	vk::AttachmentDescription attachments[] =
	{
		colorAttachment,
		depthAttachment,
		colorAttachmentResolve,
	};

	vk::RenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.attachmentCount = static_cast<uint32>(std::size(attachments));
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	renderPass = device.createRenderPass(renderPassInfo, allocator);
	
	// Create graphic pipeline
	vk::GraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.basePipelineHandle = nullptr; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	graphicsPipeline = device.createGraphicsPipeline(nullptr, { pipelineInfo }, allocator);
}

void VulkanContextLegacy::createFramebuffers()
{
	for (size_t i = 0; i < c_maxFramesInFlight; i++)
	{
		vk::ImageView attachments[] = {
			rtImageView,
			depthImageView,
			swapchainImageViews[i].get(),
		};

		vk::FramebufferCreateInfo framebufferInfo{};
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = std::size(attachments);
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		framebuffers[i] = device.createFramebuffer(framebufferInfo, allocator);
	}
}

void VulkanContextLegacy::createCommandBuffers()
{
	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.commandPool = commandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = c_maxFramesInFlight;
	commandBuffers = device.allocateCommandBuffers(allocInfo);
}

void VulkanContextLegacy::createVertexBuffer()
{
	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	bufferInfo.size = sizeof(Vertex) * std::size(vertices);
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	
	vertexBuffer = device.createBuffer(bufferInfo, allocator);

	auto const memRequirement = device.getBufferMemoryRequirements(vertexBuffer);
	
	vk::MemoryAllocateInfo allocInfo = {};
	allocInfo.allocationSize = memRequirement.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirement.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	vertexMemory = device.allocateMemory(allocInfo, allocator);

	device.bindBufferMemory(vertexBuffer, vertexMemory, 0);
	
	void* data = device.mapMemory(vertexMemory, 0, bufferInfo.size);
		memcpy(data, vertices.data(), bufferInfo.size);
	device.unmapMemory(vertexMemory);
}

void VulkanContextLegacy::createIndexBuffer()
{
	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
	bufferInfo.size = sizeof(uint32) * std::size(indices);
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	indexBuffer = device.createBuffer(bufferInfo, allocator);

	auto const memRequirement = device.getBufferMemoryRequirements(indexBuffer);

	vk::MemoryAllocateInfo allocInfo = {};
	allocInfo.allocationSize = memRequirement.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirement.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	indexMemory = device.allocateMemory(allocInfo, allocator);

	device.bindBufferMemory(indexBuffer, indexMemory, 0);

	void* data = device.mapMemory(indexMemory, 0, bufferInfo.size);
		memcpy(data, indices.data(), bufferInfo.size);
	device.unmapMemory(indexMemory);
}

void VulkanContextLegacy::createUniformBuffers()
{
	for (int i = 0; i < c_maxFramesInFlight; i++)
	{
		vk::BufferCreateInfo bufferInfo{};
		bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		bufferInfo.size = sizeof(UniformBufferObject);
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;

		uniformBuffer[i] = device.createBuffer(bufferInfo, allocator);

		auto const memRequirement = device.getBufferMemoryRequirements(uniformBuffer[i]);

		vk::MemoryAllocateInfo allocInfo = {};
		allocInfo.allocationSize = memRequirement.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirement.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		uniformBufferMemory[i] = device.allocateMemory(allocInfo, allocator);

		device.bindBufferMemory(uniformBuffer[i], uniformBufferMemory[i], 0);
	}

}

void VulkanContextLegacy::createDescriptorPool()
{
	vk::DescriptorPoolSize poolSize[2];
	poolSize[0].type = vk::DescriptorType::eUniformBuffer;
	poolSize[0].descriptorCount = swapchainImages.size();
	poolSize[1].type = vk::DescriptorType::eCombinedImageSampler;
	poolSize[1].descriptorCount = swapchainImages.size();

	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.poolSizeCount = std::size(poolSize);
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = swapchainImages.size();

	descriptorPool = device.createDescriptorPool(poolInfo, allocator);
}

void VulkanContextLegacy::createDescriptorSets()
{
	std::vector<vk::DescriptorSetLayout> layouts(swapchainImages.size(), descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = swapchainImages.size();
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets = device.allocateDescriptorSets(allocInfo);

	for (int i = 0; i < swapchainImages.size(); i++) 
	{
		vk::DescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffer[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		vk::DescriptorImageInfo imageInfo;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		vk::WriteDescriptorSet descriptorWrites[2];
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;
		
		device.updateDescriptorSets(std::size(descriptorWrites), descriptorWrites, 0, nullptr);
	}
}

void VulkanContextLegacy::createDepthResources()
{
	vk::Format const depthFormat = findDepthFormat(physicalDevice);
	
	createImage(swapchainExtent.width, swapchainExtent.height, 
		depthFormat,
		vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, 
		vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory, 1, msaaSamples);

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = depthImage;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = depthFormat;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	depthImageView = device.createImageView(viewInfo, allocator);

	transitionImageLayout(depthImage, depthFormat, 1, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
}

void VulkanContextLegacy::createColorResources()
{
	vk::Format const colorFormat = swapchainFormat;

	createImage(swapchainExtent.width, swapchainExtent.height, colorFormat, vk::ImageTiling::eOptimal, 
		vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, 
		vk::MemoryPropertyFlagBits::eDeviceLocal, rtImage, rtImageMemory, 1, msaaSamples);

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = rtImage;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = colorFormat;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	rtImageView = device.createImageView(viewInfo, allocator);
}

void VulkanContextLegacy::createTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("assets/viking_room.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
		throw std::runtime_error("failed to load texture image!");
	miplevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	vk::DeviceSize const imageSize = texWidth * texHeight * 4;

	vk::BufferCreateInfo stagingBufferInfo;
	stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	stagingBufferInfo.size = imageSize;
	stagingBufferInfo.sharingMode = vk::SharingMode::eExclusive;

	auto stagingBuffer = device.createBufferUnique(stagingBufferInfo, allocator);
	auto const stagingMemRequirements = device.getBufferMemoryRequirements(stagingBuffer.get());

	vk::MemoryAllocateInfo stagingAllocInfo = {};
	stagingAllocInfo.allocationSize = stagingMemRequirements.size;
	stagingAllocInfo.memoryTypeIndex = findMemoryType(physicalDevice, stagingMemRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	auto stagingMemory = device.allocateMemoryUnique(stagingAllocInfo, allocator);

	device.bindBufferMemory(stagingBuffer.get(), stagingMemory.get(), 0);

	void* data;
	vkMapMemory(device, stagingMemory.get(), 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingMemory.get());

	stbi_image_free(pixels);

	createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, 
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled,
		vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureImageMemory, miplevels);
	
	transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Srgb, miplevels, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	copyBufferToImage(stagingBuffer.get(), textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	//transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Srgb, miplevels, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	generateMipmaps(textureImage, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, miplevels);
}

void VulkanContextLegacy::generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	// Check if image format supports linear blitting
	vk::FormatProperties const formatProperties = physicalDevice.getFormatProperties(imageFormat);
	
	if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
		throw std::runtime_error("texture image format does not support linear blitting!");
	
	vk::CommandBuffer const commandBuffer = beginSingleTimeCommands();

	vk::ImageMemoryBarrier barrier{};
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++) 
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(),
			{}, {}, { barrier });

		vk::ImageBlit blit{};
		blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
		blit.srcOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
		blit.dstOffsets[1] = vk::Offset3D{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
			vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);

		barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		
		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlags(),
			{}, {}, { barrier });
		
		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}
	
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlags(),
		{}, {}, { barrier });

	endSingleTimeCommands(commandBuffer);
}

void VulkanContextLegacy::createTextureImageView()
{
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = textureImage;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = vk::Format::eR8G8B8A8Srgb;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = miplevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	textureImageView = device.createImageView(viewInfo, allocator);
}

void VulkanContextLegacy::createSampler()
{
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16.0f;
	samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(miplevels);

	textureSampler = device.createSampler(samplerInfo, allocator);
}

void VulkanContextLegacy::recreateSwapchain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}
	
	device.waitIdle();

	destroySwapchain();
	
	createSwapchain();
	createGraphicsPipeline();
	createColorResources();
	createDepthResources();
	createFramebuffers();
	createCommandBuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	recordCommandBuffers();
}

void VulkanContextLegacy::recordCommandBuffers()
{
	for (int i = 0; auto const& cmdBuffer : commandBuffers) 
	{
		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits(); // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		cmdBuffer.begin(beginInfo);

		vk::RenderPassBeginInfo renderPassInfo{};
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffers[i];
		renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
		renderPassInfo.renderArea.extent = swapchainExtent;

		vk::ClearValue clearsValues[2];
		clearsValues[0].color = vk::ClearColorValue{ std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f} };
		clearsValues[1].depthStencil = vk::ClearDepthStencilValue(1.0, 0.0);
		renderPassInfo.clearValueCount = std::size(clearsValues);
		renderPassInfo.pClearValues = clearsValues;

		cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
			cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
		
			vk::Buffer vertexBuffers[] = { vertexBuffer };
			vk::DeviceSize offsets[] = { 0 };
			cmdBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
			cmdBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32); 
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
		
			cmdBuffer.drawIndexed(indices.size(), 1, 0, 0, 0);
			cmdBuffer.endRenderPass();
		cmdBuffer.end();
		
		i++;
	}
}

void VulkanContextLegacy::updateUniformBuffer(int32 currentFrame)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto const currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(60.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(device, uniformBufferMemory[currentFrame], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformBufferMemory[currentFrame]);
}

void VulkanContextLegacy::drawFrame()
{
	device.waitForFences(1, &inFlightFences[currentFrame], true, UINT64_MAX);

	vk::ResultValue<uint32_t> const nextImageResult = device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailableSemaphore[currentFrame], vk::Fence{});

	if (nextImageResult.result == vk::Result::eErrorOutOfDateKHR || resized)
	{
		resized = false;
		recreateSwapchain();
		return;
	}
	else if (nextImageResult.result != vk::Result::eSuccess && nextImageResult.result != vk::Result::eSuboptimalKHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	uint32 const imageIndex = nextImageResult.value;

	updateUniformBuffer(currentFrame);
	
	vk::SubmitInfo submitInfo{};
	vk::Semaphore waitSemaphores[] = { imageAvailableSemaphore[currentFrame] };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	vk::Semaphore signalSemaphores[] = { renderFinishedSemaphore[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	device.resetFences(1, &inFlightFences[currentFrame]);
	graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]);

	vk::PresentInfoKHR presentInfo{};

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	
	vk::SwapchainKHR swapChains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentQueue.presentKHR(presentInfo);
	
	currentFrame = (currentFrame + 1) % c_maxFramesInFlight;
}

void VulkanContextLegacy::destroyBuffers()
{
	device.destroyBuffer(vertexBuffer, allocator);
	device.freeMemory(vertexMemory, allocator);

	device.destroyBuffer(indexBuffer, allocator);
	device.freeMemory(indexMemory, allocator);
}

void VulkanContextLegacy::destroyTextureImage()
{
	device.destroyImage(textureImage, allocator);
	device.freeMemory(textureImageMemory, allocator);

	device.destroyImageView(textureImageView, allocator);
}

void VulkanContextLegacy::destroySyncPrimitives()
{
	for (int i = 0; i < c_maxFramesInFlight; i++)
	{
		device.destroySemaphore(imageAvailableSemaphore[i], allocator);
		device.destroySemaphore(renderFinishedSemaphore[i], allocator);

		device.destroyFence(inFlightFences[i], allocator);
	}
}

void VulkanContextLegacy::destroySwapchain()
{
	// destroy ubo
	for (int i = 0; i < c_maxFramesInFlight; i++)
	{
		device.destroyBuffer(uniformBuffer[i], allocator);
		device.freeMemory(uniformBufferMemory[i], allocator);
	}
	
	for (int i = 0; i < c_maxFramesInFlight; i++)
	{
		device.destroyFramebuffer(framebuffers[i], allocator);
	}

	device.freeCommandBuffers(commandPool, c_maxFramesInFlight, commandBuffers.data());
	device.destroyPipeline(graphicsPipeline, allocator);
	device.destroyPipelineLayout(pipelineLayout, allocator);
	device.destroyRenderPass(renderPass, allocator);

	device.destroySwapchainKHR(swapchain, allocator);
	swapchainImageViews.clear();
	device.destroyDescriptorPool(descriptorPool, allocator);
}

void VulkanContextLegacy::destroy()
{
	device.waitIdle();

	destroySwapchain();

	destroyTextureImage();
	device.destroySampler(textureSampler, allocator);
	
	destroyBuffers();
	device.destroyDescriptorSetLayout(descriptorSetLayout, allocator);
	device.destroyCommandPool(commandPool, allocator);
	
	destroySyncPrimitives();
	instance.destroySurfaceKHR(surface, allocator);
	device.destroy(allocator);
	instance.destroy(allocator);
}
