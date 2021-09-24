#include "vulkanContext.hpp"
#include <GLFW/glfw3.h>

#include "utility.hpp"
#include "vkhShader.hpp"

static std::span<uint8> toSpan(std::vector<char> const& vec)
{
	return std::span((uint8*)vec.data(), vec.size());
}

VulkanContext::VulkanContext(GLFWwindow* win) : window(win)
{
	const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
	instance.create("ice renderer", "iceEngine", validationLayers, nullptr);
	createSurface();
	const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	deviceContext.create(instance, surface, extensions);
	swapchain.create(&deviceContext, window, surface, maxFramesInFlight, false);

	auto const vertSpv = readBinFile("shaders/vert.spv");
	auto const fragSpv = readBinFile("shaders/frag.spv");

	vk::SampleCountFlagBits const msaaSamples = vk::SampleCountFlagBits::e1;
	vk::Format const colorFormat = swapchain.swapchainFormat;
	graphicsPipeline.create(deviceContext, toSpan(vertSpv), toSpan(fragSpv),
		vkh::createDefaultRenderPass(deviceContext, colorFormat, msaaSamples),
		swapchain.swapchainExtent, msaaSamples);
}

VulkanContext::~VulkanContext()
{
	graphicsPipeline.destroy();
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

