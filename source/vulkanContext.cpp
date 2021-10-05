#include "vulkanContext.hpp"
#include <GLFW/glfw3.h>

#include "utility.hpp"
#include "vkhShader.hpp"
#include "vkhUtility.hpp"
#include "mesh.hpp"


VulkanContext::VulkanContext(GLFWwindow* win) : window(win)
{
	const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
	const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	
	instance.create("ice renderer", "iceEngine", validationLayers, nullptr);
	createSurface();
	deviceContext.create(instance, surface, extensions);
	swapchain.create(&deviceContext, window, surface, maxFramesInFlight, vsync);

	std::system("cd .\\shaders && shadercompile.bat");

	auto const vertSpv = readBinFile("shaders/vert.spv");
	auto const fragSpv = readBinFile("shaders/frag.spv");

	vk::Format const colorFormat = swapchain.format;
	defaultRenderPass = vkh::createDefaultRenderPass(deviceContext, colorFormat, msaaSamples);
	graphicsPipeline.create(deviceContext, toSpan<uint8>(vertSpv), toSpan<uint8>(fragSpv), *defaultRenderPass, swapchain.extent, msaaSamples);
	createMsResources();
	createDepthResources();
	createFramebuffers();
	commandBuffers.create(deviceContext, maxFramesInFlight);
	createSyncResources();
	createDescriptorPool();
}

VulkanContext::~VulkanContext()
{
	deviceContext.device.waitIdle();
	
	descriptorPool.reset();
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

void VulkanContext::createDescriptorPool()
{
	vk::DescriptorPoolSize poolSize[2];
	poolSize[0].type = vk::DescriptorType::eUniformBuffer;
	poolSize[0].descriptorCount = swapchain.images.size();
	poolSize[1].type = vk::DescriptorType::eCombinedImageSampler;
	poolSize[1].descriptorCount = swapchain.images.size();

	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.poolSizeCount = std::size(poolSize);
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = swapchain.images.size();

	descriptorPool = deviceContext.device.createDescriptorPoolUnique(poolInfo, deviceContext.allocationCallbacks);
}

void VulkanContext::recreateSwapchain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	deviceContext.device.waitIdle();

	swapchain.destroy();

	swapchain.create(&deviceContext, window, surface, maxFramesInFlight, vsync);

	auto const vertSpv = readBinFile("shaders/vert.spv");
	auto const fragSpv = readBinFile("shaders/frag.spv");

	vk::Format const colorFormat = swapchain.format;
	defaultRenderPass = vkh::createDefaultRenderPass(deviceContext, colorFormat, msaaSamples);
	graphicsPipeline.create(deviceContext, toSpan<uint8>(vertSpv), toSpan<uint8>(fragSpv), *defaultRenderPass, swapchain.extent, msaaSamples);
	createMsResources();
	createDepthResources();
	createFramebuffers();
}

bool VulkanContext::startFrame()
{
	deviceContext.device.waitForFences(1, &inFlightFences[currentFrame].get(), true, UINT64_MAX);

	vk::ResultValue<uint32_t> const nextImageResult = deviceContext.device.acquireNextImageKHR(*swapchain.swapchain, UINT64_MAX, *imageAvailableSemaphores[currentFrame], vk::Fence{});

	if (nextImageResult.result == vk::Result::eErrorOutOfDateKHR || resized)
	{
		resized = false;
		recreateSwapchain();
		return false;
	}
	else if (nextImageResult.result != vk::Result::eSuccess && nextImageResult.result != vk::Result::eSuboptimalKHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	imageIndex = nextImageResult.value;

	return true;
}

void VulkanContext::endFrame()
{
	vk::SubmitInfo submitInfo{};
	vk::Semaphore waitSemaphores[] = { *imageAvailableSemaphores[currentFrame] };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	
	submitInfo.commandBufferCount = 1;

	submitInfo.pCommandBuffers = &commandBuffers.commandBuffers[currentFrame].get();

	vk::Semaphore signalSemaphores[] = { *renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	deviceContext.device.resetFences(1, &inFlightFences[currentFrame].get());
	deviceContext.graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame].get());

	vk::PresentInfoKHR presentInfo{};

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	vk::SwapchainKHR swapChains[] = { *swapchain.swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	deviceContext.presentQueue.presentKHR(presentInfo);
	
	currentFrame = (currentFrame + 1) % maxFramesInFlight;
}
