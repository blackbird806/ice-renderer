#pragma once

#include "vkhTexture.hpp"
#include "vkhBuffer.hpp"
#include "vkhGraphicsPipeline.hpp"
#include "vulkanContext.hpp"

class Skybox
{
public:
	void create(VulkanContext& ctx, vk::RenderPass renderPass, const char* texturePath);
	void draw(vk::CommandBuffer cmd);

//private:
	vkh::GraphicsPipeline pipeline;
	vkh::Buffer uniformBuffer;
	vk::DescriptorSet descriptorSet;
	vkh::Buffer unitCubeVertexBuffer;
	vkh::Buffer unitCubeIndexBuffer;
	vkh::Texture skyboxTexture;
};