#include <GLFW/glfw3.h>

#include "vulkanContext.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "GUILayer.hpp"
#include "imgui/imgui.h"

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

	Material mtrl;
	mtrl.parameters["brightness"].value = 1.0f;
	mtrl.graphicsPipeline = &context.defaultPipeline;
	mtrl.descriptorSets = context.defaultPipeline.createDescriptorSets(*context.descriptorPool, vkh::Material, context.maxFramesInFlight);
	mtrl.create(context.deviceContext);
	mtrl.updateBuffer();
	mtrl.updateDescriptorSets();
	
	vk::ClearValue clearsValues[2];
	clearsValues[0].color = vk::ClearColorValue{ std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f} };
	clearsValues[1].depthStencil = vk::ClearDepthStencilValue(1.0, 0.0);
	
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		
		if (!context.startFrame())
			continue;
		
		gui.startFrame();
		auto cmdBuffer = context.commandBuffers.begin(context.currentFrame);

		ImGui::ColorEdit3("ClearValue", (float*)&clearsValues[0].color, ImGuiColorEditFlags_PickerHueWheel);
		
		vk::RenderPassBeginInfo renderPassInfo{};
		renderPassInfo.renderPass = *context.defaultRenderPass;
		renderPassInfo.framebuffer = *context.framebuffers[context.currentFrame];
		renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
		renderPassInfo.renderArea.extent = context.swapchain.extent;
		renderPassInfo.clearValueCount = std::size(clearsValues);
		renderPassInfo.pClearValues = clearsValues;
		
		cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		
			cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipeline);
			vk::DescriptorSet sets1[] = { frameSets[context.currentFrame],  mtrl.descriptorSets[context.currentFrame] };
			vk::DescriptorSet sets2[] = { modelSets[context.currentFrame] };
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipelineLayout, 0, std::size(sets1), sets1, 0, nullptr);
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipelineLayout, 2, std::size(sets2), sets2, 0, nullptr);
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
