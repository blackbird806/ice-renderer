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
			// no init ctor
			explicit CreateInfo() = default;
			
			// initialize the create info with default parameters
			CreateInfo(vkh::ShaderModule&& vertexShader_, vkh::ShaderModule&& fragmentShader_, vk::RenderPass renderPass_, vk::Extent2D imageExtent);
			
			vkh::ShaderModule vertexShader;
			vkh::ShaderModule fragmentShader;
			vk::RenderPass renderPass;

			vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
			vk::Viewport viewport;
			vk::Rect2D scissor;
			vk::PipelineViewportStateCreateInfo viewportState;
			vk::PipelineRasterizationStateCreateInfo rasterizer;
			vk::PipelineMultisampleStateCreateInfo multisampling;
			vk::PipelineDepthStencilStateCreateInfo depthStencil;
			vk::PipelineColorBlendAttachmentState colorBlendAttachment;
			vk::PipelineColorBlendStateCreateInfo colorBlending;
			// dynamic state ?
			// base handle
		};
		
		void create(vkh::DeviceContext& ctx, CreateInfo const& createInfo);
		// TODO allow copy pipelines
		// pipeline cache ?
		
		std::vector<vk::DescriptorSet> createDescriptorSets(vk::DescriptorPool pool, uint32 setIndex, uint32 count);
		void destroy();
		
		vkh::DeviceContext* deviceContext;

		vkh::ShaderDescriptorLayout dsLayout;
		
		vk::UniquePipeline pipeline;
		vk::UniquePipelineLayout pipelineLayout;
	};
	
}
