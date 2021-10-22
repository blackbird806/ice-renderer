#include "material.hpp"
#include "vkhBuffer.hpp"
#include "vkhGraphicsPipeline.hpp"
#include "vkhDeviceContext.hpp"
#include "imgui/imgui.h"
#include "utility.hpp"

// @REVIEW Material impl

void Material::create(vkh::DeviceContext& deviceContext, vkh::GraphicsPipeline& pipeline, vk::DescriptorPool descriptorPool)
{
	graphicsPipeline = &pipeline;

	for (auto const& reflectedDescriptor : pipeline.dsLayout.reflectedDescriptors)
	{
		if (reflectedDescriptor.setNumber == vkh::DescriptorSetIndex::Materials)
		{
			for (auto const& binding : reflectedDescriptor.bindings)
			{
				parameters.push_back(binding.element);
			}
		}
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
		if (ImGui::DragFloat(param.name.c_str(), (float*)&f, 0.01f, 0.0f, 1.0f))
		{
			param.value = f;
		}
	}

	void operator()(uint32 f)
	{
		int32 i = (uint32)f;
		if (ImGui::InputInt(param.name.c_str(), &i))
		{
			param.value = (uint32)i;
		}
	}

	void operator()(glm::vec3 f)
	{
		//ImGui::DragFloat3(param.name.c_str(), (float*)&param, 0.01f, 0.0f, 1.0f);
		if (ImGui::ColorEdit3(param.name.c_str(), (float*)&f))
		{
			param.value = f;
		}
	}

	void operator()(auto f)
	{
		ImGui::Text(param.name.c_str());
	}

	void operator()(vkh::ShaderReflector::ReflectedDescriptorSet::Struct& s)
	{
		for (auto& m : s.members)
		{
			std::visit(ImguiMaterialVisitor(m), m.value);
		}
	}

	vkh::ShaderReflector::ReflectedDescriptorSet::Member& param;
};

void Material::imguiEditor()
{
	for (auto& param : parameters)
	{
		std::visit(ImguiMaterialVisitor(param), param.value);
	}
}

void Material::updateMember(void* bufferData, size_t& offset, vkh::ShaderReflector::ReflectedDescriptorSet::Member const& mem)
{
	if (mem.typeFlags & SPV_REFLECT_TYPE_FLAG_ARRAY)
	{
		// array/structs inside materials are unsuported for now
		assert(false);
	}
	if (mem.typeFlags & SPV_REFLECT_TYPE_FLAG_STRUCT)
	{
		for (auto const& e : std::get<vkh::ShaderReflector::ReflectedDescriptorSet::Struct>(mem.value).members)
		{
			updateMember(bufferData, offset, e);
		}
	}
	else
	{
		size_t const size = mem.getSize();
		memcpy((uint8*)bufferData + offset, &mem.value, size);
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
