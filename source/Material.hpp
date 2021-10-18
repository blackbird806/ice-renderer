#pragma once
#include <unordered_map>
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
	void create(vkh::DeviceContext& deviceContext);

	size_t getUniformBufferSize() const noexcept;
	
	void update();
	
	void bind(vk::CommandBuffer cmdBuffer, uint32 index);
	
	void updateDescriptorSets();
	
	vkh::GraphicsPipeline* graphicsPipeline;

	std::unordered_map<std::string, vkh::ShaderReflector::DescriptorSetDescriptor::Member> parameters;
	
	std::vector<vk::DescriptorSet> descriptorSets;
	vkh::Buffer uniformBuffer;
};
