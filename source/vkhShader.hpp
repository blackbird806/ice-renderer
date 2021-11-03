#pragma once

#include <span>
#include <SPIRV-Reflect/spirv_reflect.h>
#include <vulkan/vulkan.hpp>

#include <optional>
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
		Sampler1D, Sampler2D, Sampler3D,
		ShaderStruct
	};
	
#define FROM_TYPE_SPECIALISATION_DECL(type) ShaderVarType fromType(type);
	
		FROM_TYPE_SPECIALISATION_DECL(bool)
		FROM_TYPE_SPECIALISATION_DECL(float)
		FROM_TYPE_SPECIALISATION_DECL(double)
		FROM_TYPE_SPECIALISATION_DECL(int8)
		FROM_TYPE_SPECIALISATION_DECL(int16)
		FROM_TYPE_SPECIALISATION_DECL(int32)
		FROM_TYPE_SPECIALISATION_DECL(int64)
		FROM_TYPE_SPECIALISATION_DECL(uint8)
		FROM_TYPE_SPECIALISATION_DECL(uint16)
		FROM_TYPE_SPECIALISATION_DECL(uint32)
		FROM_TYPE_SPECIALISATION_DECL(uint64)
		FROM_TYPE_SPECIALISATION_DECL(glm::vec1)
		FROM_TYPE_SPECIALISATION_DECL(glm::vec2)
		FROM_TYPE_SPECIALISATION_DECL(glm::vec3)
		FROM_TYPE_SPECIALISATION_DECL(glm::vec4)
		FROM_TYPE_SPECIALISATION_DECL(glm::mat2)
		FROM_TYPE_SPECIALISATION_DECL(glm::mat3)
		FROM_TYPE_SPECIALISATION_DECL(glm::mat4)
		FROM_TYPE_SPECIALISATION_DECL(glm::mat2x3)
		FROM_TYPE_SPECIALISATION_DECL(glm::mat3x2)
		FROM_TYPE_SPECIALISATION_DECL(glm::mat4x2)
		FROM_TYPE_SPECIALISATION_DECL(glm::mat2x4)
		FROM_TYPE_SPECIALISATION_DECL(glm::mat3x4)
		FROM_TYPE_SPECIALISATION_DECL(glm::mat4x3)
		FROM_TYPE_SPECIALISATION_DECL(glm::mat4x4)
		FROM_TYPE_SPECIALISATION_DECL(ShaderSampler1D)
		FROM_TYPE_SPECIALISATION_DECL(ShaderSampler2D)
		FROM_TYPE_SPECIALISATION_DECL(ShaderSampler3D)
		FROM_TYPE_SPECIALISATION_DECL(ShaderStruct)

#undef FROM_TYPE_SPECIALISATION_DECL

	size_t shaderVarTypeSize(ShaderVarType t);

	struct ShaderVariable
	{
		template<typename R, typename V>
		R visit(V&& visitor)
		{
			using enum vkh::ShaderVarType;
			switch (type)
			{
			case Float:
			case Vec1:
				return visitor(value<float>());
			case Double:
				return visitor(value<double>());
			case Int8:
				return visitor(value<int8>());
			case Int16:
				return visitor(value<int16>());
			case Int32:
				return visitor(value<int32>());
			case Int64:
				return visitor(value<int64>());
			case Uint8:
				return visitor(value<uint8>());
			case Uint16:
				return visitor(value<uint16>());
			case Uint32:
				return visitor(value<uint32>());
			case Uint64:
				return visitor(value<uint64>());
			case Vec2:
				return visitor(value<glm::vec2>());
			case Vec3:
				return visitor(value<glm::vec3>());
			case Vec4:
				return visitor(value<glm::vec4>());
			case Mat2x2:
				return visitor(value<glm::mat2>());
			case Mat3x3:
				return visitor(value<glm::mat3>());
			case Mat4x4:
				return visitor(value<glm::mat4>());
			case Mat2x3:
				return visitor(value<glm::mat2x3>());
			case Mat2x4:
				return visitor(value<glm::mat2x4>());
			case Mat3x2:
				return visitor(value<glm::mat3x2>());
			case Mat4x2:
				return visitor(value<glm::mat4x2>());
			case Mat3x4:
				return visitor(value<glm::mat3x4>());
			case Mat4x3:
				return visitor(value<glm::mat4x3>());
			case Sampler1D:
				return visitor(value<vkh::ShaderSampler1D>());
			case Sampler2D:
				return visitor(value<vkh::ShaderSampler2D>());
			case Sampler3D:
				return visitor(value<vkh::ShaderSampler3D>());
			case ShaderStruct:
				return visitor(*structType);
			default:;
			}
			throw std::runtime_error("unhandled type");
		}

		//helper function to access first elem of arrayElements userfull when var isn't an array
		template<typename T>
		T const& value() const
		{
			return arrayElements.get<T>(0);
		}

		template<typename T>
		T& value()
		{
			return arrayElements.get<T>(0);
		}

		// create shader variable from value
		template<typename T>
		void build(T const& v)
		{
			type = fromType(v);
			arrayElements.resize<T>(1);
			value<T>() = v;
		}
		
		size_t getSize() const;

		std::string name;
		SpvReflectTypeFlags typeFlags = 0;
		SpvReflectArrayTraits arrayTraits{};
		bool ignore = false;

		ShaderVarType type;

		// @TODO structType / arrayElements should be a variant
		std::optional<ShaderStruct> structType;
		// @improve: better version of vector variant (out of scope for now)
		VectorAny arrayElements;
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
