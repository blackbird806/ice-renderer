#pragma once

#include <vector>
#include <unordered_map>

#include "vkhBuffer.hpp"
#include "vkhGraphicsPipeline.hpp"

struct PipelineBatch
{
	static std::unordered_map<std::string, vkh::ShaderVariable> defaultPipelineConstants;

	void create(vkh::GraphicsPipeline& pipeline_, vk::DescriptorPool pool, uint32 batchSize_);

	void updatePipelineConstantsSet() const;
	void updatePipelineConstantBuffer();

	void addImageInfo(uint32 binding, vk::DescriptorImageInfo const& info);
	
	void updateTextureDescriptorSet();
	
	vk::DeviceSize getPipelineConstantsBufferEntrySize() const;

	uint32 batchSize;
	
	vkh::GraphicsPipeline* pipeline;

	vkh::Buffer pipelineConstantBuffer;
	// @Review big material buffer ? (bindless ? dynamic offset ?)
	
	std::vector<std::vector<vk::DescriptorImageInfo>> imageInfosArray;
	// @Review delete descriptors sets ?
	vk::DescriptorSet pipelineConstantsSet;
	vk::DescriptorSet texturesSet;
};
