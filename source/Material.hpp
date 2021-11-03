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

//@Review material builder ?
struct Material
{
	void create(vkh::DeviceContext& deviceContext, vkh::GraphicsPipeline& pipeline, vk::DescriptorPool);

	size_t getUniformBufferSize() const noexcept;

	void imguiEditor();
	
	void updateMember(void* bufferData, size_t& offset, vkh::ShaderVariable const& mem);
	void updateBuffer();
	
	void updateDescriptorSets();

	vkh::ShaderVariable* getParameter(std::string const& name);
	
	vkh::GraphicsPipeline* graphicsPipeline; // To remove ?

	std::vector<vkh::ShaderVariable> parameters;
	
	vk::DescriptorSet descriptorSet;
	vkh::Buffer uniformBuffer; // TODO set big material buffer in pipeline batch
};

void updateFromObjMaterial(tinyobj::material_t const& objMtrl, Material& mtrl);
