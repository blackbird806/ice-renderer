#pragma once

#include <vulkan/vulkan.hpp>

namespace vkh
{
	class ShaderReflector;
	struct DeviceContext;

	struct DescriptorSetLayout
	{
		enum SetIndex : size_t
		{
			FrameConstants = 0,
			Textures,
			Material,
			DrawCall,

			MaxSets
		};

		using LayoutBindings_t = std::array<std::vector<vk::DescriptorSetLayoutBinding>, MaxSets>;
		
		void create(vkh::DeviceContext& ctx, ShaderReflector const& vertData, ShaderReflector const& fragData);

		void destroy();
		
		LayoutBindings_t layoutBindings;
		std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
		DeviceContext* deviceContext;
	};

}
