#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "ice.hpp"
#include "vkhBuffer.hpp"
#include "vkhShader.hpp"

namespace tinyobj {
	struct material_t;
}

namespace vkh {
	struct GraphicsPipeline;
}

struct MaterialParameter
{
	template<typename R, typename V>
	R visit(V&& visitor) const
	{
		using enum vkh::ShaderVarType;
		switch(shaderVar.type)
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
		case ShaderSampler1D:
			return visitor(value<vkh::ShaderSampler1D>());
		case ShaderSampler2D:
			return visitor(value<vkh::ShaderSampler2D>());
		case ShaderSampler3D:
			return visitor(value<vkh::ShaderSampler3D>());
		case ShaderStruct:
			return visitor(value<vkh::ShaderStruct>());
		default: ;
		}
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
		// value may be modified so ensure space is available
		// @Review
		arrayElements.resize<T>(1);
		return arrayElements.get<T>(0);
	}
	vkh::ShaderVariable shaderVar;
	// @improve: better version of vector variant (out of scope for now)
	VectorAny arrayElements;
};

//@Review material builder ?
struct Material
{
	void create(vkh::DeviceContext& deviceContext, vkh::GraphicsPipeline& pipeline, vk::DescriptorPool);

	size_t getUniformBufferSize() const noexcept;

	void imguiEditor();
	
	void updateMember(void* bufferData, size_t& offset, MaterialParameter const& mem);
	void updateBuffer();
	
	void updateDescriptorSets();

	MaterialParameter* getParameter(std::string const& name);
	
	vkh::GraphicsPipeline* graphicsPipeline; // To remove ?

	std::vector<MaterialParameter> parameters;
	
	vk::DescriptorSet descriptorSet;
	vkh::Buffer uniformBuffer; // TODO set big material buffer in pipeline batch
};

void updateFromObjMaterial(tinyobj::material_t const& objMtrl, Material& mtrl);
