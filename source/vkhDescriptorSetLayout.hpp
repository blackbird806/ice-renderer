#pragma once

#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <span>
#include "vkhShader.hpp"

namespace vkh
{
	struct DeviceContext;

	enum DescriptorSetIndex : uint32
	{
		Default = 0,
		PipelineConstants = 0,
		Materials = 0,
		DrawCall = 1,

		MaxSets
	};
	
	struct ShaderDescriptorLayout
	{
		void create(vkh::DeviceContext& ctx, std::span<ShaderReflector const*> shadersInfos);

		void destroy();

		std::vector<ShaderReflector::ReflectedDescriptorSet> reflectedDescriptors;
		
		std::unordered_map<uint32, vk::UniqueDescriptorSetLayout> descriptorSetLayouts;
		DeviceContext* deviceContext;
	};

}
