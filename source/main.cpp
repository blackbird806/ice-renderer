#include <iostream>
#include <GLFW/glfw3.h>

#include "vulkanContext.hpp"
#include "mesh.hpp"


static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto& vkContext = *static_cast<VulkanContext*>(glfwGetWindowUserPointer(window));
	vkContext.resized = true;
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	
	VulkanContext context(window);
	glfwSetWindowUserPointer(window, &context);

	Mesh mesh(context.deviceContext, loadObj("assets/cube.obj"));
	mesh.material.graphicsPipeline = &context.defaultPipeline;
	mesh.material.descriptorSets = context.defaultPipeline.createDescriptorSets(*context.descriptorPool, context.maxFramesInFlight);
	mesh.material.setBuffer(mesh.uniformBuffer);
	mesh.material.updateDescriptorSets();
	
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		
		if (!context.startFrame())
			continue;
		
		auto cmdBuffer = context.commandBuffers.begin(context.currentFrame);

		vk::RenderPassBeginInfo renderPassInfo{};
		renderPassInfo.renderPass = *context.defaultRenderPass;
		renderPassInfo.framebuffer = *context.framebuffers[context.currentFrame];
		renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
		renderPassInfo.renderArea.extent = context.swapchain.extent;

		vk::ClearValue clearsValues[2];
		clearsValues[0].color = vk::ClearColorValue{ std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f} };
		clearsValues[1].depthStencil = vk::ClearDepthStencilValue(1.0, 0.0);
		renderPassInfo.clearValueCount = std::size(clearsValues);
		renderPassInfo.pClearValues = clearsValues;

		cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
			cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipeline);
			mesh.draw(cmdBuffer, context.currentFrame);

		cmdBuffer.endRenderPass();
		cmdBuffer.end();
		context.endFrame();
	}
	
	glfwTerminate();
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
