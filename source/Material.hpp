#pragma once
#include <vector>
#include <variant>
#include <vulkan/vulkan.hpp>

#include "ice.hpp"

namespace vkh {
	struct GraphicsPipeline;
	class Buffer;
}

struct Material
{
	struct WriteDescriptorSetInfos
	{
		using WriteInfo_t = std::variant<vk::DescriptorBufferInfo, vk::DescriptorImageInfo>;
		std::vector<vk::WriteDescriptorSet> descriptorsWrite;
		std::vector<WriteInfo_t> writeInfos;
	};
	
	void setBuffer(vkh::Buffer const& buffer);
	void setBuffer(std::string const& name, vkh::Buffer const& buffer);

	void bind(vk::CommandBuffer cmdBuffer, uint32 index);
	
	void updateDescriptorSets();
	
	vkh::GraphicsPipeline* graphicsPipeline;
	std::vector<vk::DescriptorSet> descriptorSets;
	WriteDescriptorSetInfos descriptorsWriteInfos;
};
