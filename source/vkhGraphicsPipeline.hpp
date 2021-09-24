#include <vulkan/vulkan.hpp>
#include <span>

#include "ice.hpp"

namespace vkh
{
	struct DeviceContext;

	vk::UniqueRenderPass createDefaultRenderPass(vkh::DeviceContext& deviceContext, vk::Format colorFormat, vk::SampleCountFlagBits msaaSamples);
	
	struct GrpahicsPipeline
	{
		void create(vkh::DeviceContext& deviceContext, std::span<uint8> vertSpv, std::span<uint8> fragSpv, vk::UniqueRenderPass&& renderPass_, vk::Extent2D imageExtent, vk::SampleCountFlagBits msaaSamples);

		void destroy();

		std::vector<vk::UniqueDescriptorSetLayout> descriptorSetLayouts;
		vk::UniquePipeline pipeline;
		vk::UniquePipelineLayout pipelineLayout;
		vk::UniqueRenderPass renderPass;
	};
	
}