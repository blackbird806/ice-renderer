#pragma once

#include <vector>
#include "vkhGraphicsPipeline.hpp"
#include "vkhBuffer.hpp"
#include "vkhTexture.hpp"

struct Material;

struct PipelineBatch
{
	void create(vkh::GraphicsPipeline& pipeline, uint32 bufferCount);
	void destroy();

	void createDescriptorPool(uint32 bufferCount);
	void destroyDescriptorPool();

	void bind(vk::CommandBuffer cmdBuff);
	
	vkh::GraphicsPipeline* pipeline;
	std::vector<Material> materials;

	vk::UniqueDescriptorPool descriptorPool;
	
	std::vector<vk::DescriptorSet> pipelineConstantSets;
	std::vector<vkh::Buffer> pipelineConstantBuffers;

	std::vector<vk::DescriptorSet> pipelineMaterialsSets;
	std::vector<vkh::Buffer> pipelineMaterialsBuffers;

	std::vector<vk::DescriptorSet> pipelineTexturesSets;
	std::vector<vkh::Texture> pipelineTextures;
};
