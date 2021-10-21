#pragma once

#include <vector>

#include "vkhBuffer.hpp"
#include "vkhGraphicsPipeline.hpp"
#include "vkhTexture.hpp"

struct PipelineBatch
{
	void create(vkh::GraphicsPipeline& pipeline_, vk::DescriptorPool pool, uint32 batchSize_);

	void updatePipelineConstantsSet() const;
	
	vk::DeviceSize getPipelineConstantsBufferEntrySize() const;

	uint32 batchSize;
	
	// @Review should own of the pipeline ?
	vkh::GraphicsPipeline* pipeline;

	vkh::Buffer pipelineConstantBuffer;
	// @Review big material buffer ? (bindless ? dynamic offset ?)
	
	std::vector<std::vector<vkh::Texture>> textureArrays;

	vk::DescriptorSet pipelineConstantsSet;
	vk::DescriptorSet texturesSet;
};
