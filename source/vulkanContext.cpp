#include "vulkanContext.hpp"
#include <GLFW/glfw3.h>

VulkanContext::VulkanContext(GLFWwindow* win) : window(win)
{
	const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
	instance.create("ice renderer", "iceEngine", validationLayers, nullptr);
	createSurface();
	const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	deviceContext.create(instance, surface, extensions);
}

VulkanContext::~VulkanContext()
{
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



void VulkanContext::createSwapchain()
{


}
