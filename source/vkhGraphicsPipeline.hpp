#pragma once

#include <vulkan/vulkan.hpp>

#include "ice.hpp"
#include "vkhDescriptorSetLayout.hpp"

namespace vkh
{
	struct DeviceContext;

	vk::UniqueRenderPass createDefaultRenderPassMSAA(vkh::DeviceContext& deviceContext, vk::Format colorFormat, vk::SampleCountFlagBits msaaSamples);
	
	struct GraphicsPipeline
	{
		struct CreateInfo
		{
			vkh::ShaderModule vertexShader;
			vkh::ShaderModule fragmentShader;
			vk::RenderPass renderPass;
			vk::Extent2D imageExtent;
			vk::SampleCountFlagBits msaaSamples;
		};
		
		void create(vkh::DeviceContext& ctx, CreateInfo const& createInfo);

		std::vector<vk::DescriptorSet> createDescriptorSets(vk::DescriptorPool pool, vkh::DescriptorSetIndex setIndex, uint32 count);
		void destroy();
		
		vkh::DeviceContext* deviceContext;

		vkh::ShaderDescriptorLayout dsLayout;
		
		vk::UniquePipeline pipeline;
		vk::UniquePipelineLayout pipelineLayout;
	};
	
}
