#include <iostream>
#include <chrono>
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
#include "utility.hpp"
#include "imguiCustomWidgets.hpp"

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

static auto buildPipeline(VulkanContext& context, vk::RenderPass renderPass, vkh::ShaderModule& vertexShader, vkh::ShaderModule& fragmentShader)
{
	int width, height;
	glfwGetWindowSize(context.window, &width, &height);
	vkh::GraphicsPipeline::CreateInfo pipelineInfo = {
		.vertexShader = std::move(vertexShader),
		.fragmentShader = std::move(fragmentShader),
		.renderPass = renderPass,
		.imageExtent = { (uint32)width, (uint32)height},
		.msaaSamples = context.msaaSamples
	};

	vkh::GraphicsPipeline pipeline;
	pipeline.create(context.deviceContext, pipelineInfo);
	return pipeline;
}

static auto buildDefaultPipelineAndRenderPass(VulkanContext& context)
{
	vk::Format const colorFormat = context.swapchain.format;
	auto defaultRenderPass = vkh::createDefaultRenderPassMSAA(context.deviceContext, colorFormat, context.msaaSamples);

	std::system("cd .\\shaders && shadercompile.bat");

	auto const fragSpv = readBinFile("shaders/frag.spv");
	vkh::ShaderModule fragmentShader;
	fragmentShader.create(context.deviceContext, toSpan<uint8>(fragSpv));
	auto const vertSpv = readBinFile("shaders/vert.spv");

	vkh::ShaderModule vertexShader;
	vertexShader.create(context.deviceContext, toSpan<uint8>(vertSpv));

	int width, height;
	glfwGetWindowSize(context.window, &width, &height);
	vkh::GraphicsPipeline::CreateInfo pipelineInfo = {
		.vertexShader = std::move(vertexShader),
		.fragmentShader = std::move(fragmentShader),
		.renderPass = *defaultRenderPass,
		.imageExtent = { (uint32)width, (uint32)height},
		.msaaSamples = context.msaaSamples
	};

	vkh::GraphicsPipeline defaultPipeline;
	defaultPipeline.create(context.deviceContext, pipelineInfo);

	struct
	{
		vkh::GraphicsPipeline pipeline;
		vk::UniqueRenderPass renderPass;
	} ret;
	ret.pipeline = std::move(defaultPipeline);
	ret.renderPass = std::move(defaultRenderPass);
	return ret;
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
	
	auto [defaultPipeline, defaultRenderPass] = buildDefaultPipelineAndRenderPass(context);
	vkh::ShaderModule fragmentShader;
	fragmentShader.create(context.deviceContext, toSpan<uint8>(readBinFile("shaders/frag2.spv")));
	vkh::ShaderModule vertexShader;
	vertexShader.create(context.deviceContext, toSpan<uint8>(readBinFile("shaders/vert2.spv")));
	
	auto newPipeline = buildPipeline(context, *defaultRenderPass, vertexShader, fragmentShader);
	auto presentFrameBuffers = context.createPresentFramebuffers(*defaultRenderPass);

	context.onSwapchainRecreate = [&] ()
	{
		auto pair = buildDefaultPipelineAndRenderPass(context);
		defaultPipeline = std::move(pair.pipeline);
		defaultRenderPass = std::move(pair.renderPass);
		//newPipeline = buildPipeline(context, *defaultRenderPass, vertexShader, fragmentShader);
		presentFrameBuffers = context.createPresentFramebuffers(*defaultRenderPass);
		gui.handleSwapchainRecreation(context);
	};

	Mesh mesh(context.deviceContext, loadObj("assets/cube.obj"), context.maxFramesInFlight);
	auto const obj = loadObj("assets/palm_long.obj");
	Mesh mesh2(context.deviceContext, obj, context.maxFramesInFlight);

	vkh::Texture text = loadTexture(context.deviceContext, "assets/texture.jpg");
	vkh::Texture text2 = loadTexture(context.deviceContext, "assets/grass.png");

	PipelineBatch defaultPipelineBatch;
	defaultPipelineBatch.create(defaultPipeline, *context.descriptorPool, 32);

	mesh.modelSets = defaultPipeline.createDescriptorSets(*context.descriptorPool, vkh::DrawCall, context.maxFramesInFlight);
	mesh2.modelSets = defaultPipeline.createDescriptorSets(*context.descriptorPool, vkh::DrawCall, context.maxFramesInFlight);

	for(int i = 0; i < context.maxFramesInFlight; i++)
	{
		vk::DescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = mesh.modelBuffer.buffer;
		bufferInfo.offset = i * sizeof(glm::mat4);
		bufferInfo.range = sizeof(glm::mat4);

		vk::DescriptorBufferInfo bufferInfo2{};
		bufferInfo2.buffer = mesh2.modelBuffer.buffer;
		bufferInfo2.offset = i * sizeof(glm::mat4);
		bufferInfo2.range = sizeof(glm::mat4);

		vk::WriteDescriptorSet descriptorWrites[2];
		descriptorWrites[0].dstSet = mesh.modelSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].dstSet = mesh2.modelSets[i];
		descriptorWrites[1].dstBinding = 0;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &bufferInfo2;
		
		context.deviceContext.device.updateDescriptorSets(std::size(descriptorWrites), descriptorWrites, 0, nullptr);
	}

	vk::DescriptorImageInfo imageInfo{};
	imageInfo.sampler = *text.sampler;
	imageInfo.imageView = *text.imageView;
	imageInfo.imageLayout = text.image.getLayout();

	vk::DescriptorImageInfo imageInfo2{};
	imageInfo2.sampler = *text2.sampler;
	imageInfo2.imageView = *text2.imageView;
	imageInfo2.imageLayout = text2.image.getLayout();

	defaultPipelineBatch.addImageInfo(0, imageInfo);
	defaultPipelineBatch.addImageInfo(1, imageInfo2);
	defaultPipelineBatch.updateTextureDescriptorSet();
	
	Material mtrl;
	mtrl.create(context.deviceContext, defaultPipeline, *context.descriptorPool);
	mtrl.updateBuffer();
	mtrl.updateDescriptorSets();
	scene.materials.push_back(std::move(mtrl));

	Material mtrl2;
	mtrl2.create(context.deviceContext, newPipeline, *context.descriptorPool);
	mtrl2.updateBuffer();
	mtrl2.updateDescriptorSets();
	scene.materials.push_back(std::move(mtrl2));
	
	for (auto const& objMaterial : obj.materials)
	{
		Material mtrlObj;
		mtrlObj.create(context.deviceContext, defaultPipeline, *context.descriptorPool);
		updateFromObjMaterial(objMaterial, mtrlObj);
		mtrlObj.updateBuffer();
		mtrlObj.updateDescriptorSets();
		scene.materials.push_back(std::move(mtrlObj));
	}
	
	vk::ClearValue clearsValues[2];
	clearsValues[0].color = vk::ClearColorValue{ std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f} };
	clearsValues[1].depthStencil = vk::ClearDepthStencilValue(1.0, 0.0);

	scene.meshes.push_back(std::move(mesh));
	scene.meshes.push_back(std::move(mesh2));
	
	auto root = scene.addObject(invalidNodeID)
		.setName("root")
		.setLocalMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)));

	scene.addObject(root.nodeId)
		.setName("cube")
		.setLocalMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)))
		.setRenderObject(RenderObject{ .pipelineID = 0, .materialID = 0, .meshID = 0 });
	
	scene.addObject(root.nodeId)
		.setName("boat")
		.setLocalMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 0.2f, 0.2f)))
		.setRenderObject(RenderObject{ .pipelineID = 0, .materialID = 1, .meshID = 1 });

	float time = 0;
	float deltaTime = 0;
	glm::vec3 a;
	while (!glfwWindowShouldClose(window))
	{
		auto startFramePoint = std::chrono::high_resolution_clock::now();

		glfwPollEvents();
		
		if (!context.startFrame())
			continue;
		
		gui.startFrame();
		auto cmdBuffer = context.commandBuffers.begin(context.currentFrame);

		ImGui::Text("deltaTime %f", deltaTime);
		ImGui::ColorEdit3("ClearValue", (float*)&clearsValues[0].color, ImGuiColorEditFlags_PickerHueWheel);
		
		if (ImGui::Button("Rebuild pipelines"))
		{
			context.recreateSwapchain();
			ImGui::EndFrame();
			continue;
		}

		ImGui::Begin("Scene");
		scene.imguiDrawSceneTree();
		ImGui::End();

		ImGui::Begin("Inspector");
		scene.imguiDrawInspector();
		ImGui::End();
		
		scene.computeWorldsTransforms();
		
		for (int i = 0; auto& m : scene.materials)
		{
			ImGui::PushID(i++);
			m.imguiEditor();
			m.updateBuffer();
			ImGui::PopID();
			ImGui::Separator();
		}

		glm::mat4 view;
		glm::mat4 proj;
		view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		proj = glm::perspective(glm::radians(60.0f), (float)context.swapchain.extent.width / context.swapchain.extent.height, 0.1f, 10.0f);
		proj[1][1] *= -1;

		PipelineBatch::defaultPipelineConstants["view"] = { .value = view };
		PipelineBatch::defaultPipelineConstants["proj"] = { .value = proj };
		PipelineBatch::defaultPipelineConstants["time"] = { .value = time };

		defaultPipelineBatch.updatePipelineConstantBuffer();
		
		vk::RenderPassBeginInfo renderPassInfo{};
		renderPassInfo.renderPass = *defaultRenderPass;
		renderPassInfo.framebuffer = *presentFrameBuffers[context.currentFrame];
		renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
		renderPassInfo.renderArea.extent = context.swapchain.extent;
		renderPassInfo.clearValueCount = std::size(clearsValues);
		renderPassInfo.pClearValues = clearsValues;
		
		cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		
			cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *defaultPipeline.pipeline);

			vk::DescriptorSet sets0[] = { defaultPipelineBatch.pipelineConstantsSet };
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *defaultPipeline.pipelineLayout, 0, std::size(sets0), sets0, 0, nullptr);
			vk::DescriptorSet sets1[] = { defaultPipelineBatch.texturesSet };
			cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *defaultPipeline.pipelineLayout, 1, std::size(sets1), sets1, 0, nullptr);

			uint32 lastMaterialID = -1;
			for (auto const& [id, object] : scene.renderObjects)
			{
				scene.meshes[object.meshID].modelBuffer.writeStruct(scene.worlds[id], sizeof(glm::mat4) * context.currentFrame);

				if (object.materialID != lastMaterialID)
				{
					vk::DescriptorSet sets2[] = { scene.materials[object.materialID].descriptorSet };
					cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *defaultPipeline.pipelineLayout, 2, std::size(sets2), sets2, 0, nullptr);
					lastMaterialID = object.materialID;
				}
				
				vk::DescriptorSet sets3[] = { scene.meshes[object.meshID].modelSets[context.currentFrame] };
				cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *defaultPipeline.pipelineLayout, 3, std::size(sets3), sets3, 0, nullptr);
				scene.meshes[object.meshID].draw(cmdBuffer);
			}
		
		cmdBuffer.endRenderPass();
		
		gui.render(cmdBuffer, context.currentFrame, context.swapchain.extent);
		cmdBuffer.end();

		context.endFrame();
		
		deltaTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - startFramePoint).count();
		time += deltaTime;
	}
	
	// wait idle before destroying gui
	context.deviceContext.device.waitIdle();
	gui.destroy();
	glfwTerminate();
	return 0;
}
