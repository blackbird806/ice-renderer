#pragma once

#include <vulkan/vulkan.hpp>
#include "vkhShader.hpp"

namespace vkh
{
	struct DeviceContext;

	struct DescriptorSetLayout
	{
		enum SetIndex : size_t
		{
			PipelineConstants = 0,
			Textures,
			Material,
			DrawCall,

			MaxSets
		};

		using LayoutBindings_t = std::array<std::vector<vk::DescriptorSetLayoutBinding>, MaxSets>;
		
		void create(vkh::DeviceContext& ctx, ShaderReflector const& vertData, ShaderReflector const& fragData);

		void destroy();

		std::vector<ShaderReflector::DescriptorSetDescriptor> descriptorsDescriptors;
		
		LayoutBindings_t layoutBindings;
		std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
		DeviceContext* deviceContext;
	};

}
