#pragma once

#include <vector>
#include "vkhGraphicsPipeline.hpp"
#include "vkhBuffer.hpp"

struct Material;

struct PipelineBatch
{
	void create(vkh::GraphicsPipeline& pipeline, uint32 bufferCount);
	void destroy();

	void createDescriptorPool(uint32 bufferCount);
	void destroyDescriptorPool();
	
	vkh::GraphicsPipeline* pipeline;
	std::vector<Material> materials;

	vk::UniqueDescriptorPool descriptorPool;
	
	std::vector<vk::DescriptorSet> pipelineConstantSets;
	std::vector<vkh::Buffer> pipelineConstantBuffers;
};
