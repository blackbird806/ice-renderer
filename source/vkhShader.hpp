#pragma once

#include <span>
#include <SPIRV-Reflect/spirv_reflect.h>
#include <vulkan/vulkan.hpp>

#include <variant>
#include <glm/glm.hpp>

#include "ice.hpp"
#include "utility.hpp"

namespace vkh
{
	struct DeviceContext;

	static size_t constexpr uniformBufferAllignement = 16;

	struct ShaderStruct
	{
		size_t getSize() const;

		std::string name;
		std::vector<struct ShaderVariable> members;
	};

	struct ShaderSampler1D { };
	struct ShaderSampler2D { };
	struct ShaderSampler3D { };
	
	enum class ShaderVarType
	{
		Float, Double, Bool, 
		Int8, Int16, Int32, Int64,
		Uint8, Uint16, Uint32, Uint64,
		Vec1, Vec2, Vec3, Vec4,
		Mat2x2, Mat3x3, Mat4x4,
		Mat2x3, Mat2x4, Mat3x2,
		Mat4x2, Mat3x4, Mat4x3,
		ShaderSampler1D, ShaderSampler2D, ShaderSampler3D,
		ShaderStruct
	};

	size_t shaderVarTypeSize(ShaderVarType t);

	struct ShaderVariable
	{

		size_t getSize() const;

		std::string name;
		SpvReflectTypeFlags typeFlags = 0;
		SpvReflectArrayTraits arrayTraits{};
		ShaderVarType type;
		std::optional<ShaderStruct> structType;
		
		bool ignore = false;


	};

	
	class ShaderReflector
	{
	public:
		ShaderReflector() = default;
		ShaderReflector(ShaderReflector&&) noexcept;
		ShaderReflector& operator=(ShaderReflector&&) noexcept;
		
		void create(std::span<uint8 const> spvCode);
		void destroy();
		~ShaderReflector();

		struct ReflectedDescriptorSet
		{
			struct Binding
			{
				ShaderVariable element;
				SpvReflectDescriptorType descriptorType;
			};

			bool operator==(ReflectedDescriptorSet const& rhs) const;
			
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

			bool operator==(DescriptorSetLayoutData const& rhs) const noexcept;
		};
		
		[[nodiscard]] VertexDescription getVertexDescriptions() const; 
		[[nodiscard]] std::vector<ShaderReflector::DescriptorSetLayoutData> getDescriptorSetLayoutData() const;

		std::vector<ReflectedDescriptorSet> createReflectedDescriptorSet() const;
		vk::ShaderStageFlagBits getShaderStage() const;
		
	private:
		
		std::vector<SpvReflectDescriptorSet*> reflectDescriptorSets() const;
		SpvReflectShaderModule module;
		bool isValid = false;
	};

	struct ShaderModule
	{
		void create(vkh::DeviceContext& ctx, std::span<uint8> data);
		void destroy();

		vk::PipelineShaderStageCreateInfo getPipelineShaderStage() const;

		vk::UniqueShaderModule module;
		ShaderReflector reflector;
	};
	
}
