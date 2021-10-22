#pragma once

#include <vector>

#include "vkhBuffer.hpp"
#include "vkhGraphicsPipeline.hpp"
#include "vkhTexture.hpp"

struct PipelineBatch
{
	void create(vkh::GraphicsPipeline& pipeline_, vk::DescriptorPool pool, uint32 batchSize_);

	void updatePipelineConstantsSet() const;

	void addImageInfo(vk::DescriptorImageInfo const& info);
	void updateTextureDescriptorSet();
	
	vk::DeviceSize getPipelineConstantsBufferEntrySize() const;

	uint32 batchSize;
	
	vkh::GraphicsPipeline* pipeline;

	vkh::Buffer pipelineConstantBuffer;
	// @Review big material buffer ? (bindless ? dynamic offset ?)
	
	std::vector<vk::DescriptorImageInfo> imageInfos;

	vk::DescriptorSet pipelineConstantsSet;
	vk::DescriptorSet texturesSet;
};
