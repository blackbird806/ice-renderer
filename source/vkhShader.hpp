#pragma once

#include <span>
#include <SPIRV-Reflect/spirv_reflect.h>
#include <vulkan/vulkan.hpp>

#include <variant>
#include <glm/glm.hpp>

#include "ice.hpp"

namespace vkh
{
	struct DeviceContext;

	class ShaderReflector
	{
	public:

		ShaderReflector(std::span<uint8 const> spvCode);
		~ShaderReflector();

		struct DescriptorSetDescriptor
		{
			struct Member;
			struct Struct
			{
				size_t getSize() const;

				std::string name;
				std::vector<Member> members;
			};
			
			struct Member
			{
				struct Sampler1D { };
				struct Sampler2D { };
				struct Sampler3D { };
				
				using Type = std::variant<
					float, double,
					int8, int16, int32, int64,
					uint8, uint16, uint32, uint64,
					glm::vec1, glm::vec2, glm::vec3, glm::vec4,
					glm::mat2, glm::mat3, glm::mat4,
					glm::mat2x3, glm::mat2x4, glm::mat3x2,
					glm::mat4x2, glm::mat3x4, glm::mat4x3,
					Sampler1D, Sampler2D, Sampler3D,
					Struct
				>;
				
				std::string name;
				SpvReflectTypeFlags typeFlags = 0;
				SpvReflectArrayTraits arrayTraits{};
				
				// @improve: vector variant (out of scope for now)
				Type value;
			};

			struct Binding
			{
				Member element;
				SpvReflectDescriptorType descriptorType;
			};

			uint setNumber;
			std::vector<Binding> bindings;
		};
		
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
		
		[[nodiscard]] VertexDescription getVertexDescriptions() const; 
		[[nodiscard]] std::vector<ShaderReflector::DescriptorSetLayoutData> getDescriptorSetLayoutData() const;

		std::vector<SpvReflectDescriptorSet*> reflectDescriptorSets() const;
		std::vector<DescriptorSetDescriptor> createDescriptorSetDescriptors() const;

	private:
		SpvReflectShaderModule module;
	};

}
