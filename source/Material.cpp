#include "material.hpp"

#include <tiny/tiny_obj_loader.h>
#include <format>

#include "vkhBuffer.hpp"
#include "vkhGraphicsPipeline.hpp"
#include "vkhDeviceContext.hpp"
#include "imgui/imgui.h"
#include "utility.hpp"

// @REVIEW Material impl

void Material::create(vkh::DeviceContext& deviceContext, vkh::GraphicsPipeline& pipeline, vk::DescriptorPool descriptorPool)
{
	graphicsPipeline = &pipeline;

	for (auto const& binding : pipeline.dsLayout.reflectedDescriptors[vkh::DescriptorSetIndex::Materials].bindings)
	{
		parameters.push_back(binding.element);
	}
	
	vma::AllocationCreateInfo allocInfo;
	allocInfo.usage = vma::MemoryUsage::eCpuToGpu;
	
	vk::BufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	// @Review alignement
	bufferCreateInfo.size = getUniformBufferSize();
	bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	uniformBuffer.create(deviceContext, bufferCreateInfo, allocInfo);
	
	descriptorSet = pipeline.createDescriptorSets(descriptorPool, vkh::Materials, 1)[0];
}

size_t Material::getUniformBufferSize() const noexcept
{
	size_t bufferSize = 0;

	for (auto const& p : parameters)
		bufferSize += p.getSize();
	
	return bufferSize;
}

// @Review redo a proper architecture and clean code
struct ImguiMaterialVisitor
{
	void operator()(float f)
	{
		if (param.ignore) return;
		if (param.typeFlags & SPV_REFLECT_TYPE_FLAG_ARRAY)
		{
			for (int i = 0; i < param.getSize() / vkh::shaderVarTypeSize(param.type); i++)
			{
				float f = param.arrayElements.get<float>(i);
				if (ImGui::DragFloat(std::format("{}_{}", param.name, i).c_str(), (float*)&f, 0.01f, 0.0f, 1.0f))
				{
					param.arrayElements.get<float>(i) = f;
				}
			}
		}
		else
		{
			if (ImGui::DragFloat(param.name.c_str(), (float*)&f, 0.01f, 0.0f, 1.0f))
			{
				param.value<float>() = f;
			}
		}
	}

	void operator()(int32 i)
	{
		if (ImGui::InputInt(param.name.c_str(), &i))
		{
			param.value<int32>() = i;
		}
	}

	void operator()(glm::vec3 f)
	{
		if (ImGui::ColorEdit3(param.name.c_str(), (float*)&f))
		{
			param.value<glm::vec3>() = f;
		}
	}

	void operator()(auto f)
	{
		ImGui::Text(param.name.c_str());
	}

	void operator()(vkh::ShaderStruct& s)
	{
		for (int i = 0; auto& m : s.members)
		{
			ImGui::PushID(i++);
			m.visit<void>(ImguiMaterialVisitor{m});
			ImGui::PopID();
		}
	}
	
	vkh::ShaderVariable& param;
};

void Material::imguiEditor()
{
	for (auto& param : parameters)
	{
		param.visit<void>(ImguiMaterialVisitor{ param });
	}
}

void Material::updateMember(void* bufferData, size_t& offset, vkh::ShaderVariable const& mem)
{
	if (mem.type == vkh::ShaderVarType::ShaderStruct)
	{
		for (auto const& e : mem.structType->members)
		{
			updateMember(bufferData, offset, e);
		}
	}
	else
	{
		size_t const size = mem.arrayElements.sizeRaw();
		memcpy((uint8*)bufferData + offset, mem.arrayElements.data(), size);
		offset += size;
	}
}

//@Improve 
void Material::updateBuffer()
{
	void* bufferData = uniformBuffer.map();
	size_t offset = 0;
	for (auto const& p : parameters)
	{
		updateMember(bufferData, offset, p);
	}
	uniformBuffer.unmap();
}

void Material::updateDescriptorSets()
{
	vk::DescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = uniformBuffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;
	
	vk::WriteDescriptorSet descriptorWrite;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;
	
	graphicsPipeline->deviceContext->device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

vkh::ShaderVariable* Material::getParameter(std::string const& name)
{
	// @TODO handle this more elgantly;
	auto& trueParams = parameters[0].structType->members;
	auto const it = std::find_if(trueParams.begin(), trueParams.end(), [&name] (auto const& e)
		{
			return e.name == name;
		});
	if (it != trueParams.end())
		return &(*it);
	return nullptr;
}

void updateFromObjMaterial(tinyobj::material_t const& objMtrl, Material& mtrl)
{
#define PARAMETER_CASE_VEC3(NAME) if (auto* NAME = mtrl.getParameter(#NAME)) NAME->value<glm::vec3>() = glm::vec3(objMtrl.NAME[0], objMtrl.NAME[1], objMtrl.NAME[2]);
	PARAMETER_CASE_VEC3(ambient);
	PARAMETER_CASE_VEC3(diffuse);
	PARAMETER_CASE_VEC3(specular);
	PARAMETER_CASE_VEC3(transmittance);
	PARAMETER_CASE_VEC3(emission);

#define PARAMETER_CASE_SINGLE(NAME) if (auto* NAME = mtrl.getParameter(#NAME)) NAME->value<decltype(objMtrl.NAME)>() = objMtrl.NAME;
	PARAMETER_CASE_SINGLE(shininess);
	PARAMETER_CASE_SINGLE(ior);
	PARAMETER_CASE_SINGLE(dissolve);
	PARAMETER_CASE_SINGLE(illum);
	PARAMETER_CASE_SINGLE(roughness);
	PARAMETER_CASE_SINGLE(metallic);
	PARAMETER_CASE_SINGLE(sheen);
	PARAMETER_CASE_SINGLE(clearcoat_thickness);
	PARAMETER_CASE_SINGLE(clearcoat_roughness);
	PARAMETER_CASE_SINGLE(anisotropy);
	PARAMETER_CASE_SINGLE(anisotropy_rotation);
}
