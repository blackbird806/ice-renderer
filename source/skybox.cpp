#include "skybox.hpp"

#include <span>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

static glm::vec3 unitCubeVertices[] = {
	{-0.5f, -0.5f, 0.5f},
	{0.5f, -0.5f, 0.5f},
	{-0.5f, 0.5f, 0.5f},
	{0.5f, 0.5f, 0.5f},
	{-0.5f, 0.5f, -0.5f},
	{0.5f, 0.5f, -0.5f},
	{-0.5f, -0.5f, -0.5f},
	{0.5f, -0.5f, -0.5f},
};

static uint32 unitCubeIndices[] = {
	0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 18, 17, 19, 20, 21, 22, 22, 21, 23
};

static vkh::Texture loadSkyboxTexture(vkh::DeviceContext& ctx, std::string const& path)
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
	textureInfo.mipLevels = 1;
	textureInfo.data = std::span(pixels, imageSize);
	textureInfo.width = texWidth;
	textureInfo.height = texHeight;

	text.create(ctx, textureInfo);
	text.image.transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

	stbi_image_free(pixels);
	return text;
}

void Skybox::create(VulkanContext& context, vk::RenderPass renderPass, const char* texturePath)
{
	auto const skyboxfragSpv = readBinFile("shaders/skybox_frag.spv");
	vkh::ShaderModule fragmentShader;
	fragmentShader.create(context.deviceContext, toSpan<uint8>(skyboxfragSpv));
	auto const skyboxvertSpv = readBinFile("shaders/skybox_vert.spv");
	vkh::ShaderModule vertexShader;
	vertexShader.create(context.deviceContext, toSpan<uint8>(skyboxvertSpv));

	int width, height;
	glfwGetWindowSize(context.window, &width, &height);
	vkh::GraphicsPipeline::CreateInfo pipelineInfo = {
		.vertexShader = std::move(vertexShader),
		.fragmentShader = std::move(fragmentShader),
		.renderPass = renderPass,
		.imageExtent = { (uint32)width, (uint32)height},
		.msaaSamples = context.msaaSamples
	};

	pipeline.create(context.deviceContext, pipelineInfo);
	
	skyboxTexture = loadSkyboxTexture(context.deviceContext, texturePath);
	
	{
		vk::BufferCreateInfo bufferInfo;
		bufferInfo.size = sizeof(unitCubeVertices);
		bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;

		vma::AllocationCreateInfo allocInfo;
		allocInfo.usage = vma::MemoryUsage::eGpuOnly;

		unitCubeVertexBuffer.createWithStaging(context.deviceContext, bufferInfo, allocInfo, std::span((uint8*)unitCubeVertices, sizeof(unitCubeVertices)));
	}
	{
		vk::BufferCreateInfo bufferInfo;
		bufferInfo.size = sizeof(unitCubeIndices);
		bufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;

		vma::AllocationCreateInfo allocInfo;
		allocInfo.usage = vma::MemoryUsage::eGpuOnly;

		unitCubeIndexBuffer.createWithStaging(context.deviceContext, bufferInfo, allocInfo, std::span((uint8*)unitCubeIndices, sizeof(unitCubeIndices)));
	}
	{
		vk::BufferCreateInfo bufferInfo;
		bufferInfo.size = sizeof(unitCubeIndices);
		bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;

		vma::AllocationCreateInfo allocInfo;
		allocInfo.usage = vma::MemoryUsage::eCpuToGpu;

		uniformBuffer.create(context.deviceContext, bufferInfo, allocInfo);
	}
	descriptorSet = pipeline.createDescriptorSets(*context.descriptorPool, vkh::Default, 1)[0];

	vk::DescriptorBufferInfo uniformBufferInfo{};
	uniformBufferInfo.buffer = uniformBuffer.buffer;
	uniformBufferInfo.offset = 0;
	uniformBufferInfo.range = VK_WHOLE_SIZE;

	vk::WriteDescriptorSet uniformWrites[2];
	uniformWrites[0].dstSet = descriptorSet;
	uniformWrites[0].dstBinding = 0;
	uniformWrites[0].dstArrayElement = 0;
	uniformWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
	uniformWrites[0].descriptorCount = 1;
	uniformWrites[0].pBufferInfo = &uniformBufferInfo;

	vk::DescriptorImageInfo textureInfo;
	textureInfo.sampler = *skyboxTexture.sampler;
	textureInfo.imageView = *skyboxTexture.imageView;
	textureInfo.imageLayout = skyboxTexture.image.getLayout();
	
	uniformWrites[1].dstSet = descriptorSet;
	uniformWrites[1].dstBinding = 1;
	uniformWrites[1].dstArrayElement = 0;
	uniformWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	uniformWrites[1].descriptorCount = 1;
	uniformWrites[1].pImageInfo = &textureInfo;
	
	context.deviceContext.device.updateDescriptorSets(std::size(uniformWrites), uniformWrites, 0, nullptr);
}

void Skybox::draw(vk::CommandBuffer cmd)
{
	// @TODO register skybox draw commands ahead of time
	// this will need command buffer and vk context refactor
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);
	
	vk::DescriptorSet sets[] = { descriptorSet };
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.pipelineLayout, 0, std::size(sets), sets, 0, nullptr);
	cmd.bindVertexBuffers(0, { unitCubeVertexBuffer.buffer }, {0});
	cmd.bindIndexBuffer(unitCubeIndexBuffer.buffer, 0, vk::IndexType::eUint32);

	cmd.drawIndexed(cubeIndexCount, 1, 0, 0, 0);
}
