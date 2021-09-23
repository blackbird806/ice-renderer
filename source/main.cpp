#include <iostream>
#include <GLFW/glfw3.h>

#include "vulkanContext.hpp"

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	
	VulkanContext context(window);
	
	return 0;
}

// OLD MAIN
//#include "vulkanContextLegacy.hpp"
//
//static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
//	auto const app = static_cast<VulkanContextLegacy*>(glfwGetWindowUserPointer(window));
//	app->resized = true;
//}
//
//int main()
//{
//	std::cout << "Hello World!\n";
//	
//	glfwInit();
//	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//	GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
//	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
//	
//	uint32_t extensionCount = 0;
//	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
//
//	std::cout << extensionCount << " extensions supported\n";
//
//	VulkanContextLegacy context;
//	glfwSetWindowUserPointer(window, &context);
//	context.window = window;
//
//	const char* validationLayers[]{ "VK_LAYER_KHRONOS_validation" };
//	const char* deviceExtensions[] { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
//
//	context.loadModel();
//	context.createInstance(validationLayers);
//	context.createSurface();
//	context.createDevice(deviceExtensions);
//	context.createSyncPrimitives();
//	context.createCommandPool();
//	context.createSwapchain();
//	context.createGraphicsPipeline();
//	context.createColorResources();
//	context.createDepthResources();
//	context.createFramebuffers();
//	context.createCommandBuffers();
//	context.createVertexBuffer();
//	context.createIndexBuffer();
//	context.createUniformBuffers();
//	context.createTextureImage();
//	context.createTextureImageView();
//	context.createSampler();
//	context.createDescriptorPool();
//	context.createDescriptorSets();
//	context.recordCommandBuffers();
//	
//	while (!glfwWindowShouldClose(window))
//	{
//		glfwPollEvents();
//		context.drawFrame();
//	}
//	
//	glfwDestroyWindow(window);
//	
//	context.destroy();
//	glfwTerminate();
//
//	return 0;
//}
//
