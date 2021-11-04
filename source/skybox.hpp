#pragma once

#include "vkhTexture.hpp"
#include "vkhBuffer.hpp"
#include "vkhGraphicsPipeline.hpp"
#include "vulkanContext.hpp"
#include "pipelineBatch.hpp"

class Skybox
{
public:
	void create(VulkanContext& ctx, vk::RenderPass renderPass, const char* texturePath);
	void draw(vk::CommandBuffer cmd);

private:
	vkh::GraphicsPipeline pipeline;
	PipelineBatch pipelineBatch;
	vkh::Buffer unitCubeVertexBuffer;
	vkh::Buffer unitCubeIndexBuffer;
	vkh::Texture skyboxTexture;
	static constexpr uint32 cubeIndexCount = 8;
};