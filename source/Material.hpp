#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "ice.hpp"
#include "vkhBuffer.hpp"
#include "vkhShader.hpp"
#include "vkhGraphicsPipeline.hpp"
#include "vkhTexture.hpp"

namespace tinyobj {
	struct material_t;
}

struct Material
{
	void create(vkh::DeviceContext& deviceContext, vkh::GraphicsPipeline&& pipeline, vk::DescriptorPool);

	void imguiEditor();
	
	void updateMember(void* bufferData, size_t& offset, vkh::ShaderVariable const& mem);
	void updateBuffer();
	
	void updateDescriptorSets();

	template<typename T>
	void setParameter(std::string const& name, T val)
	{
		auto* param = getParameter(name);
		if (!param)
			return;

		param->build<T>(val);
	}

	//template<typename T>
	//void setConstant(std::string const& name, T val)
	//{
	//	auto* param = getConstant(name);
	//	if (!param)
	//		return;

	//	param->build<T>(val);
	//}

	void setTextureParameter(std::string const& name, vkh::Texture& texture);

	void bind(vk::CommandBuffer cmd);
	
	struct TextureBindingElement
	{
		uint32 binding;
		vkh::Texture* texture;
	};
	
	vkh::ShaderVariable* getParameter(std::string const& name);
	//vkh::ShaderVariable* getConstant(std::string const& name);
	
	vkh::GraphicsPipeline graphicsPipeline;

private:
	
	[[nodiscard]] size_t getUniformBufferSize() const noexcept;
	void computeParametersSize();
	size_t pipelineConstantsSize = 0, parametersSize = 0;
	
	std::vector<vkh::ShaderVariable> parameters;
	//std::vector<vkh::ShaderVariable> constants;
	
	vk::DescriptorSet parametersSet;
	//vk::DescriptorSet constantsSet; // @Review use another buffer for constants ?
	vkh::Buffer uniformBuffer; 
	std::unordered_map<std::string, TextureBindingElement> paramsTextures;
};

void updateFromObjMaterial(tinyobj::material_t const& objMtrl, Material& mtrl);
