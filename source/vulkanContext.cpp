#include "vulkanContext.hpp"
#include <GLFW/glfw3.h>

#include "utility.hpp"
#include "vkhShader.hpp"
#include "vkhUtility.hpp"
#include "mesh.hpp"

static std::span<uint8> toSpan(std::vector<char> const& vec)
{
	return std::span((uint8*)vec.data(), vec.size());
}

VulkanContext::VulkanContext(GLFWwindow* win) : window(win)
{
	auto loadedMesh = loadObj("assets/viking_room.obj");
	
	const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
	instance.create("ice renderer", "iceEngine", validationLayers, nullptr);
	createSurface();
	const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	deviceContext.create(instance, surface, extensions);
	swapchain.create(&deviceContext, window, surface, maxFramesInFlight, false);

	auto const vertSpv = readBinFile("shaders/vert.spv");
	auto const fragSpv = readBinFile("shaders/frag.spv");

	vk::Format const colorFormat = swapchain.format;
	defaultRenderPass = vkh::createDefaultRenderPass(deviceContext, colorFormat, msaaSamples);
	graphicsPipeline.create(deviceContext, toSpan(vertSpv), toSpan(fragSpv), *defaultRenderPass, swapchain.extent, msaaSamples);
	createMsResources();
	createDepthResources();
	createFramebuffers();
	commandBuffers.create(deviceContext, maxFramesInFlight);
	createSyncResources();
}

VulkanContext::~VulkanContext()
{
	renderFinishedSemaphores.clear();
	imageAvailableSemaphores.clear();
	inFlightFences.clear();
	commandBuffers.destroy();
	msImageView.reset();
	depthImageView.reset();
	depthImage.destroy();
	msImage.destroy();
	framebuffers.clear();
	graphicsPipeline.destroy();
	defaultRenderPass.reset();
	swapchain.destroy();
	deviceContext.destroy();
	instance.handle->destroySurfaceKHR(surface);
	instance.destroy();
}

void VulkanContext::createSurface()
{
	assert(window);
	VkSurfaceKHR tmpSurface;
	glfwCreateWindowSurface(*instance.handle, window, reinterpret_cast<VkAllocationCallbacks*>(instance.allocationCallbacks), &tmpSurface);
	surface = tmpSurface;
}

void VulkanContext::createMsResources()
{
	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent.width = swapchain.extent.width;
	imageInfo.extent.height = swapchain.extent.width;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = swapchain.format;
	imageInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;
	imageInfo.samples = msaaSamples;

	vma::AllocationCreateInfo allocInfo;
	allocInfo.usage = vma::MemoryUsage::eGpuOnly;
	
	msImage.create(deviceContext, imageInfo, allocInfo);

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = msImage.handle;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = msImage.imageInfo.format;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	msImageView = deviceContext.device.createImageViewUnique(viewInfo, deviceContext.allocationCallbacks);
}

void VulkanContext::createDepthResources()
{
	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent.width = swapchain.extent.width;
	imageInfo.extent.height = swapchain.extent.width;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = vkh::findDepthFormat(deviceContext.physicalDevice);
	imageInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;
	imageInfo.samples = msaaSamples;

	vma::AllocationCreateInfo allocInfo;
	allocInfo.usage = vma::MemoryUsage::eGpuOnly;

	depthImage.create(deviceContext, imageInfo, allocInfo);

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = depthImage.handle;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = depthImage.imageInfo.format;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	depthImageView = deviceContext.device.createImageViewUnique(viewInfo, deviceContext.allocationCallbacks);
}

void VulkanContext::createFramebuffers()
{
	framebuffers.reserve(maxFramesInFlight);
	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		vk::ImageView attachments[] = {
			*msImageView,
			*depthImageView,
			*swapchain.imageViews[i],
		};

		vk::FramebufferCreateInfo framebufferInfo{};
		framebufferInfo.renderPass = *defaultRenderPass;
		framebufferInfo.attachmentCount = std::size(attachments);
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapchain.extent.width;
		framebufferInfo.height = swapchain.extent.height;
		framebufferInfo.layers = 1;

		framebuffers.push_back(deviceContext.device.createFramebufferUnique(framebufferInfo, deviceContext.allocationCallbacks));
	}
}

void VulkanContext::createSyncResources()
{
	imageAvailableSemaphores.reserve(maxFramesInFlight);
	renderFinishedSemaphores.reserve(maxFramesInFlight);
	inFlightFences.reserve(maxFramesInFlight);

	for (int i = 0; i < maxFramesInFlight; i++)
	{
		vk::SemaphoreCreateInfo semaphoreCreateInfo{};
		imageAvailableSemaphores.push_back(deviceContext.device.createSemaphoreUnique(semaphoreCreateInfo, deviceContext.allocationCallbacks));
		renderFinishedSemaphores.push_back(deviceContext.device.createSemaphoreUnique(semaphoreCreateInfo, deviceContext.allocationCallbacks));

		vk::FenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		inFlightFences.push_back(deviceContext.device.createFenceUnique(fenceCreateInfo, deviceContext.allocationCallbacks));
	}
}
