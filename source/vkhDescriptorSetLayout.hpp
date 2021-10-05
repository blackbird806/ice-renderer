#pragma once

#include <vulkan/vulkan.hpp>

namespace vkh
{
	class ShaderReflector;
	struct DeviceContext;

	struct DescriptorSetLayout
	{
		void create(vkh::DeviceContext& ctx, ShaderReflector const& vertData, ShaderReflector const& fragData);

		void destroy();
		
		std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
		std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
		DeviceContext* deviceContext;
	};

}
