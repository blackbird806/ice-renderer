#pragma once

#include <span>
#include <SPIRV-Reflect/spirv_reflect.h>
#include <vulkan/vulkan.hpp>

#include "ice.hpp"

namespace vkh
{
	struct DeviceContext;

	class ShaderReflector
	{
	public:
		ShaderReflector(std::span<uint8 const> spvCode);
		~ShaderReflector();

		struct VertexDescription
		{
			std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
			vk::VertexInputBindingDescription bindingDescription;
		};

		struct DescriptorSetLayoutData
		{
			uint32 set_number;
			vk::DescriptorSetLayoutCreateInfo create_info;
			std::vector<vk::DescriptorSetLayoutBinding> bindings;
		};
		
		[[nodiscard]] VertexDescription getVertexDescriptions();
		[[nodiscard]] DescriptorSetLayoutData getDescriptorSetLayoutData();
		
		SpvReflectShaderModule module;
	};

	struct Shader
	{
		void create(DeviceContext* deviceContext, std::span<uint8 const> spvCode);

		void destroy();
		
		vk::UniqueShaderModule shaderModule;
	};
}
