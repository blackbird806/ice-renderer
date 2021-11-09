#pragma once

#include <vector>
#include <unordered_map>

#include "vkhBuffer.hpp"
#include "vkhGraphicsPipeline.hpp"

namespace vkh {
	struct Texture;
}

struct PipelineBatch
{
	static std::unordered_map<std::string, vkh::ShaderVariable> defaultPipelineConstants;

	void create(vkh::GraphicsPipeline& pipeline_, vk::DescriptorPool pool);

	void updatePipelineConstantsSet() const;
	void updatePipelineConstantBuffer();

	int32 addTexture(uint32 binding, vkh::Texture const& text);
	
	void updateTextureDescriptorSet();
	
	vk::DeviceSize getPipelineConstantsBufferEntrySize() const;

	vkh::GraphicsPipeline* pipeline;

	vkh::Buffer pipelineConstantBuffer;
	// @Review big material buffer ? (bindless ? dynamic offset ?)
	
	std::vector<std::vector<vk::DescriptorImageInfo>> imageInfosArray;
	// @Review delete descriptors sets ?
	vk::DescriptorSet pipelineConstantsSet;
	vk::DescriptorSet texturesSet;
};
