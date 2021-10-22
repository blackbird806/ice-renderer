#pragma once

#include <vector>

#include "vkhBuffer.hpp"
#include "vkhGraphicsPipeline.hpp"
#include "vkhTexture.hpp"

struct PipelineBatch
{
	void create(vkh::GraphicsPipeline& pipeline_, vk::DescriptorPool pool, uint32 batchSize_);

	void updatePipelineConstantsSet() const;

	void addImageInfo(uint32 binding, vk::DescriptorImageInfo const& info);
	void setImageArraySize(size_t size);
	
	void updateTextureDescriptorSet();
	
	vk::DeviceSize getPipelineConstantsBufferEntrySize() const;

	uint32 batchSize;
	
	vkh::GraphicsPipeline* pipeline;

	vkh::Buffer pipelineConstantBuffer;
	// @Review big material buffer ? (bindless ? dynamic offset ?)
	
	std::vector<std::vector<vk::DescriptorImageInfo>> imageInfosArray;

	vk::DescriptorSet pipelineConstantsSet;
	vk::DescriptorSet texturesSet;
};
