#pragma once

#include <vector>
#include "vkhGraphicsPipeline.hpp"
#include "vkhBuffer.hpp"
#include "vkhTexture.hpp"

struct Material;

struct PipelineBatch
{
	using MaterialID_t = uint32;
	
	void create(vkh::GraphicsPipeline& pipeline, uint32 bufferCount);
	void destroy();

	void createDescriptorPool(uint32 bufferCount);
	void destroyDescriptorPool();

	void bindMaterial(vk::CommandBuffer cmdBuff, MaterialID_t matId, uint32 frameIndex);
	
	vkh::GraphicsPipeline* pipeline;

	vk::UniqueDescriptorPool descriptorPool;
	
	std::vector<vk::DescriptorSet> pipelineConstantSets;
	std::vector<vkh::Buffer> pipelineConstantBuffers;

	std::vector<vk::DescriptorSet> pipelineMaterialsSets;
	std::vector<vkh::Buffer> pipelineMaterialsBuffers;

	std::vector<vk::DescriptorSet> pipelineTexturesSets;
	std::vector<vkh::Texture> pipelineTextures;
};
