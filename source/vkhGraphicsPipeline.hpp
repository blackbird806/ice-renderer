#pragma once

#include <vulkan/vulkan.hpp>
#include <span>

#include "ice.hpp"
#include "vkhDescriptorSetLayout.hpp"

namespace vkh
{
	struct DeviceContext;

	vk::UniqueRenderPass createDefaultRenderPassMSAA(vkh::DeviceContext& deviceContext, vk::Format colorFormat, vk::SampleCountFlagBits msaaSamples);
	
	struct GraphicsPipeline
	{
		void create(vkh::DeviceContext& ctx, std::span<uint8> vertSpv, std::span<uint8> fragSpv, vk::RenderPass renderPass, vk::Extent2D imageExtent, vk::SampleCountFlagBits msaaSamples);

		std::vector<vk::DescriptorSet> createDescriptorSets(vk::DescriptorPool pool, vkh::DescriptorSetLayout::SetIndex setIndex, uint32 count);
		void destroy();
		
		vkh::DeviceContext* deviceContext;
		vkh::DescriptorSetLayout dsLayout;
		vk::UniquePipeline pipeline;
		vk::UniquePipelineLayout pipelineLayout;
	};
	
}
