#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "ice.hpp"
#include "vkhBuffer.hpp"
#include "vkhShader.hpp"

namespace vkh {
	struct GraphicsPipeline;
}

//@Review material builder ?
struct Material
{
	void create(vkh::DeviceContext& deviceContext, vkh::GraphicsPipeline& pipeline);

	size_t getUniformBufferSize() const noexcept;

	void imguiEditor();
	
	void updateMember(void* bufferData, size_t& offset, vkh::ShaderReflector::ReflectedDescriptorSet::Member const& mem);
	void updateBuffer();
	
	void bind(vk::CommandBuffer cmdBuffer, uint32 index);
	
	void updateDescriptorSets();
	
	vkh::GraphicsPipeline* graphicsPipeline;

	std::vector<vkh::ShaderReflector::ReflectedDescriptorSet::Member> parameters;
	
	std::vector<vk::DescriptorSet> descriptorSets;
	vkh::Buffer uniformBuffer;
};
