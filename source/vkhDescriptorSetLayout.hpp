#pragma once

#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <span>
#include "vkhShader.hpp"

namespace vkh
{
	struct DeviceContext;

	enum DescriptorSetIndex : size_t
	{
		Default = 0,
		Lights = 0,
		PipelineConstants,
		Textures,
		Materials,
		DrawCall,

		MaxSets
	};
	
	struct ShaderDescriptorLayout
	{
		void create(vkh::DeviceContext& ctx, std::span<ShaderReflector const*> shadersInfos);

		void destroy();

		std::vector<ShaderReflector::ReflectedDescriptorSet> reflectedDescriptors;
		
		std::unordered_map<DescriptorSetIndex, vk::UniqueDescriptorSetLayout> descriptorSetLayouts;
		DeviceContext* deviceContext;
	};

}
