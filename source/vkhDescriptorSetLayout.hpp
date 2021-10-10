#pragma once

#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include "vkhShader.hpp"

namespace vkh
{
	struct DeviceContext;

	enum DescriptorSetIndex : size_t
	{
		PipelineConstants = 0,
		Textures,
		Material,
		DrawCall,

		MaxSets
	};
	
	struct ShaderDescriptorLayout
	{
		using LayoutBindings_t = std::array<std::vector<vk::DescriptorSetLayoutBinding>, MaxSets>;
		
		void create(vkh::DeviceContext& ctx, ShaderReflector const& shaderInfos);

		void destroy();

		std::vector<ShaderReflector::DescriptorSetDescriptor> descriptorsDescriptors;
		
		LayoutBindings_t layoutBindings;
		std::unordered_map<DescriptorSetIndex, vk::UniqueDescriptorSetLayout> descriptorSetLayouts;
		DeviceContext* deviceContext;
	};

}
