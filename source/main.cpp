#include <iostream>
#include <GLFW/glfw3.h>

#include <stb/stb_image.h>
#include <imgui/imgui.h>

#include "vulkanContext.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "GUILayer.hpp"
#include "vkhTexture.hpp"
#include "pipelineBatch.hpp"
#include "scenegraph.hpp"

#undef min
#undef max

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto& vkContext = *static_cast<VulkanContext*>(glfwGetWindowUserPointer(window));
	vkContext.resized = true;
}

static vkh::Texture loadTexture(vkh::DeviceContext& ctx, std::string const& path)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
		throw std::runtime_error("failed to load texture image!");

	vk::DeviceSize const imageSize = texWidth * texHeight * 4;

	vkh::Texture text;
	vkh::Texture::CreateInfo textureInfo;
	textureInfo.format = vk::Format::eR8G8B8A8Srgb;
	textureInfo.tiling = vk::ImageTiling::eOptimal;
	// @TODO mipmaps
	textureInfo.mipLevels = 1; // static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	textureInfo.data = std::span(pixels, imageSize);
	textureInfo.width = texWidth;
	textureInfo.height = texHeight;

	text.create(ctx, textureInfo);
	text.image.transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	
	stbi_image_free(pixels);
	return text;
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	
	VulkanContext context(window);
	glfwSetWindowUserPointer(window, &context);

	Scene scene;

	GUILayer gui;
	gui.init(context);
	context.onSwapchainRecreate = [&context, &gui] ()
	{
		gui.handleSwapchainRecreation(context);
	};

	Mesh mesh(context.deviceContext, loadObj("assets/cube.obj"));
	Mesh mesh2(context.deviceContext, loadObj("assets/ship_wreck.obj"));

	vkh::Texture text = loadTexture(context.deviceContext, "assets/texture.jpg");
	vkh::Texture text2 = loadTexture(context.deviceContext, "assets/grass.png");
	
	struct FrameConstants
	{
		glm::mat4 view;
		glm::mat4 proj;
	} frameConstants;


	PipelineBatch defaultPipelineBatch;
	defaultPipelineBatch.create(context.defaultPipeline, *context.descriptorPool, 32);
	//vkh::Buffer frameConstantsBuffer;
	//{
	//	vma::AllocationCreateInfo allocInfo;
	//	allocInfo.usage = vma::MemoryUsage::eCpuToGpu;
	//	vk::BufferCreateInfo bufferCreateInfo;
	//	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	//	bufferCreateInfo.size = sizeof(FrameConstants);
	//	bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	//	frameConstantsBuffer.create(context.deviceContext, bufferCreateInfo, allocInfo);
	//	frameConstantsBuffer.writeStruct(frameConstants);
	//}

	//auto frameSets = context.defaultPipeline.createDescriptorSets(*context.descriptorPool, vkh::PipelineConstants, context.maxFramesInFlight);
	auto textureSets = context.defaultPipeline.createDescriptorSets(*context.descriptorPool, vkh::Textures, context.maxFramesInFlight);
	mesh.modelSet = context.defaultPipeline.createDescriptorSets(*context.descriptorPool, vkh::DrawCall, context.maxFramesInFlight)[0];
	mesh2.modelSet = context.defaultPipeline.createDescriptorSets(*context.descriptorPool, vkh::DrawCall, context.maxFramesInFlight)[0];

	{
		vk::DescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = mesh.modelBuffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(glm::mat4);

		vk::WriteDescriptorSet descriptorWrites[2];
		descriptorWrites[0].dstSet = mesh.modelSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].dstSet = mesh2.modelSet;
		descriptorWrites[1].dstBinding = 0;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &bufferInfo;
		
		context.deviceContext.device.updateDescriptorSets(std::size(descriptorWrites), descriptorWrites, 0, nullptr);
	}

	size_t constexpr maxTextures = 64;

	vk::DescriptorImageInfo imageInfo{};
	imageInfo.sampler = *text.sampler;
	imageInfo.imageView = *text.imageView;
	imageInfo.imageLayout = text.image.getLayout();

	vk::DescriptorImageInfo imageInfo2{};
	imageInfo2.sampler = *text2.sampler;
	imageInfo2.imageView = *text2.imageView;
	imageInfo2.imageLayout = text2.image.getLayout();
	
	std::vector<vk::DescriptorImageInfo> imageInfos(maxTextures, imageInfo);
	imageInfos[1] = imageInfo2;
	
	for (auto& set : textureSets)
	{
		vk::WriteDescriptorSet descriptorWrites[1];
		descriptorWrites[0].dstSet = set;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrites[0].descriptorCount = imageInfos.size();
		descriptorWrites[0].pImageInfo = imageInfos.data();

		context.deviceContext.device.updateDescriptorSets(std::size(descriptorWrites), descriptorWrites, 0, nullptr);
	}
	
	//for (auto& set : frameSets)	
	//{
	//	vk::DescriptorBufferInfo bufferInfo{};
	//	bufferInfo.buffer = frameConstantsBuffer.buffer;
	//	bufferInfo.offset = 0;
	//	bufferInfo.range = sizeof(FrameConstants);

	//	vk::WriteDescriptorSet descriptorWrites[1];
	//	descriptorWrites[0].dstSet = set;
	//	descriptorWrites[0].dstBinding = 0;
	//	descriptorWrites[0].dstArrayElement = 0;
	//	descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
	//	descriptorWrites[0].descriptorCount = 1;
	//	descriptorWrites[0].pBufferInfo = &bufferInfo;

	//	context.deviceContext.device.updateDescriptorSets(std::size(descriptorWrites), descriptorWrites, 0, nullptr);
	//}

	Material mtrl;
	mtrl.create(context.deviceContext, context.defaultPipeline);
	mtrl.descriptorSets = context.defaultPipeline.createDescriptorSets(*context.descriptorPool, vkh::Materials, context.maxFramesInFlight);
	mtrl.updateBuffer();
	mtrl.updateDescriptorSets();

	vk::ClearValue clearsValues[2];
	clearsValues[0].color = vk::ClearColorValue{ std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f} };
	clearsValues[1].depthStencil = vk::ClearDepthStencilValue(1.0, 0.0);

	scene.materials.push_back(std::move(mtrl));
	scene.meshes.push_back(std::move(mesh));
	scene.meshes.push_back(std::move(mesh2));
	
	auto root = scene.addObject(invalidNodeID)
		.setName("root")
		.setLocalMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)))
		.setRenderObject(RenderObject{ .pipelineID = 0, .materialID = 0, .meshID = 0 });
	
	scene.addObject(root.nodeId)
		.setName("child 1")
		.setLocalMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f)))
		.setRenderObject(RenderObject{ .pipelineID = 0, .materialID = 0, .meshID = 1 });
	
	//auto a = scene.addObject(root)
	//	.setTransform(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f)))
	//	.setRenderObject(RenderObject{ .pipelineID = 0, .materialID = 0, .meshID = 0 });
	//
	//scene.addObject(a.nodeId)
	//	.setTransform(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f)))
	//	.setRenderObject(RenderObject{ .pipelineID = 0, .materialID = 0, .meshID = 0 });

	//scene.addObject(a.nodeId)
	//	.setTransform(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f)))
	//	.setRenderObject(RenderObject{ .pipelineID = 0, .materialID = 0, .meshID = 0 });
	
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		
		if (!context.startFrame())
			continue;
		
		gui.startFrame();
		auto cmdBuffer = context.commandBuffers.begin(context.currentFrame);

		ImGui::ColorEdit3("ClearValue", (float*)&clearsValues[0].color, ImGuiColorEditFlags_PickerHueWheel);
		
		scene.imguiDrawSceneTree();
		scene.computeWorldsTransforms();
		
		for (auto& m : scene.materials)
		{
			m.imguiEditor();
			m.updateBuffer();
		}
		
		frameConstants.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		frameConstants.proj = glm::perspective(glm::radians(60.0f), (float)context.swapchain.extent.width / context.swapchain.extent.height, 0.1f, 10.0f);
		frameConstants.proj[1][1] *= -1;
		defaultPipelineBatch.pipelineConstantBuffer.writeStruct(frameConstants);

		vk::RenderPassBeginInfo renderPassInfo{};
		renderPassInfo.renderPass = *context.defaultRenderPass;
		renderPassInfo.framebuffer = *context.framebuffers[context.currentFrame];
		renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
		renderPassInfo.renderArea.extent = context.swapchain.extent;
		renderPassInfo.clearValueCount = std::size(clearsValues);
		renderPassInfo.pClearValues = clearsValues;
		
		cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		
			cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipeline);

			vk::DescriptorSet sets0[] = { defaultPipelineBatch.pipelineConstantsSet };
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipelineLayout, 0, std::size(sets0), sets0, 0, nullptr);
			vk::DescriptorSet sets1[] = { textureSets[context.currentFrame] };
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipelineLayout, 1, std::size(sets1), sets1, 0, nullptr);

			for (auto const& [id, object] : scene.renderObjects)
			{
				scene.meshes[object.meshID].modelBuffer.writeStruct(scene.worlds[id]);

				vk::DescriptorSet sets2[] = { scene.materials[object.materialID].descriptorSets[context.currentFrame] };
				vk::DescriptorSet sets3[] = { scene.meshes[object.meshID].modelSet };

				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipelineLayout, 2, std::size(sets2), sets2, 0, nullptr);
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipelineLayout, 3, std::size(sets3), sets3, 0, nullptr);
				scene.meshes[object.meshID].draw(cmdBuffer);
			}
		
			//vk::DescriptorSet sets0[] = { frameSets[context.currentFrame] };
			//vk::DescriptorSet sets0[] = { defaultPipelineBatch.pipelineConstantsSet };
			//vk::DescriptorSet sets1[] = { textureSets[context.currentFrame] };
			//vk::DescriptorSet sets2[] = { mtrl.descriptorSets[context.currentFrame] };
			//vk::DescriptorSet sets3[] = { modelSets[context.currentFrame] };
		
			//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipelineLayout, 0, std::size(sets0), sets0, 0, nullptr);
			//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipelineLayout, 1, std::size(sets1), sets1, 0, nullptr);
			//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipelineLayout, 2, std::size(sets2), sets2, 0, nullptr);
			//cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.defaultPipeline.pipelineLayout, 3, std::size(sets3), sets3, 0, nullptr);

		//mesh.draw(cmdBuffer, context.currentFrame);

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
