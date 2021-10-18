#include <GLFW/glfw3.h>

#include "vulkanContext.hpp"
#include "mesh.hpp"
#include "GUILayer.hpp"

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

	GUILayer gui;
	gui.init(context);
	context.onSwapchainRecreate = [&context, &gui]()
	{
		gui.handleSwapchainRecreation(context);
	};

	Mesh mesh(context.deviceContext, loadObj("assets/cube.obj"));

	struct FrameConstants
	{
		glm::mat4 view;
		glm::mat4 proj;
	} frameConstants;
	
	frameConstants.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	frameConstants.proj = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 10.0f);
	frameConstants.proj[1][1] *= -1;

	vkh::Buffer frameConstantsBuffer;
	{
		vma::AllocationCreateInfo allocInfo;
		allocInfo.usage = vma::MemoryUsage::eCpuToGpu;
		vk::BufferCreateInfo bufferCreateInfo;
		bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		bufferCreateInfo.size = sizeof(FrameConstants);
		bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		frameConstantsBuffer.create(context.deviceContext.gpuAllocator, bufferCreateInfo, allocInfo);
		frameConstantsBuffer.writeStruct(frameConstants);
	}

	auto frameSets = context.defaultPipeline.createDescriptorSets(*context.descriptorPool, vkh::PipelineConstants, context.maxFramesInFlight);
	auto modelSets = context.defaultPipeline.createDescriptorSets(*context.descriptorPool, vkh::DrawCall, context.maxFramesInFlight);

	for (auto& set : modelSets)
	{
		vk::DescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = mesh.modelBuffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(glm::mat4);

		vk::WriteDescriptorSet descriptorWrites[1];
		descriptorWrites[0].dstSet = set;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		context.deviceContext.device.updateDescriptorSets(std::size(descriptorWrites), descriptorWrites, 0, nullptr);
	}
	
	for (auto& set : frameSets)
	{
		vk::DescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = frameConstantsBuffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(FrameConstants);

		vk::WriteDescriptorSet descriptorWrites[1];
		descriptorWrites[0].dstSet = set;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		context.deviceContext.device.updateDescriptorSets(std::size(descriptorWrites), descriptorWrites, 0, nullptr);
	}
	
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
			vk::DescriptorSet sets[] = { frameSets[context.currentFrame], modelSets[context.currentFrame] };
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipelineLayout, 0, std::size(sets), sets, 0, nullptr);
			//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipelineLayout, 0, 1, &modelSets[context.currentFrame], 0, nullptr);
			mesh.draw(cmdBuffer, context.currentFrame);

		cmdBuffer.endRenderPass();
		
		gui.render(cmdBuffer, context.currentFrame, context.swapchain.extent);
		cmdBuffer.end();

		context.endFrame();
	}
	
	// wait idle before destroying gui
	context.deviceContext.device.waitIdle();
	gui.destroy();
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
