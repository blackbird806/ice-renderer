#include "material.hpp"

#include <tiny/tiny_obj_loader.h>
#include <format>

#include "vkhBuffer.hpp"
#include "vkhGraphicsPipeline.hpp"
#include "vkhDeviceContext.hpp"
#include "imgui/imgui.h"
#include "utility.hpp"

void Material::create(vkh::DeviceContext& deviceContext, vkh::GraphicsPipeline&& pipeline, vk::DescriptorPool descriptorPool)
{
	graphicsPipeline = std::move(pipeline);

	for (uint32 bindingNum = 0; auto const& binding : graphicsPipeline.dsLayout.reflectedDescriptors[vkh::Default].bindings)
	{
		if (vkh::isSampler(binding.element.type))
		{
			// @TODO default texture
			paramsTextures[binding.element.name] = { bindingNum, nullptr };
		}
		else
		{
			parameters.push_back(binding.element);
		}
		bindingNum++;
	}
	computeParametersSize();
	
	vma::AllocationCreateInfo allocInfo;
	allocInfo.usage = vma::MemoryUsage::eCpuToGpu;
	
	vk::BufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	bufferCreateInfo.size = parametersSize;
	bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	uniformBuffer.create(deviceContext, bufferCreateInfo, allocInfo);
	
	parametersSet = graphicsPipeline.createDescriptorSets(descriptorPool, 0, 1)[0];
}

//@Bug should be reviewd / rewritten
size_t Material::getUniformBufferSize() const noexcept
{
	size_t uniformBufferSize = 0;
	for (auto const& set : graphicsPipeline.dsLayout.reflectedDescriptors)
	{
		for (auto const& binding : set.bindings)
		{
			if (isSampler(binding.element.type))
				continue;
			
			uniformBufferSize += binding.element.getAlignedSize();
		}
	}
	return uniformBufferSize;
}

void Material::computeParametersSize()
{
	parametersSize = 0;
	for (auto const& param : parameters)
	{
		parametersSize += param.getAlignedSize();
	}
}

// @Review redo a proper architecture and clean code
struct ImguiMaterialVisitor
{
	void operator()(float f)
	{
		if (param.ignore) return;
		if (param.typeFlags & SPV_REFLECT_TYPE_FLAG_ARRAY)
		{
			for (int i = 0; i < param.getAlignedSize() / vkh::shaderVarTypeSize(param.type); i++)
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

	void operator()(uint32 b)
	{
		if (ImGui::Checkbox(param.name.c_str(), (bool*)&b))
		{
			param.value<bool>() = b;
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

	void operator()(glm::vec2 f)
	{
		if (ImGui::DragFloat2(param.name.c_str(), (float*)&f, 0.001))
		{
			param.value<glm::vec2>() = f;
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
	// samplers should not be stored among parameters
	assert(!vkh::isSampler(mem.type));
	
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

// @Improve 
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
	std::vector<vk::WriteDescriptorSet> parametersdescriptorWrites;
	
	// pipeline constants
	//vk::DescriptorBufferInfo pipelineConstantsBufferInfo{};
	//pipelineConstantsBufferInfo.buffer = uniformBuffer.buffer;
	//pipelineConstantsBufferInfo.offset = 0;
	//pipelineConstantsBufferInfo.range = pipelineConstantsSize;

	//vk::WriteDescriptorSet  pipelineConstantsBufferWrite;
	//pipelineConstantsBufferWrite.dstSet = constantsSet;
	//pipelineConstantsBufferWrite.dstBinding = 0;
	//pipelineConstantsBufferWrite.dstArrayElement = 0;
	//pipelineConstantsBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
	//pipelineConstantsBufferWrite.descriptorCount = 1;
	//pipelineConstantsBufferWrite.pBufferInfo = &pipelineConstantsBufferInfo;
	//
	//parametersdescriptorWrites.push_back(pipelineConstantsBufferWrite);
	
	// materials parameters
	{
		vk::DescriptorBufferInfo paramsbufferInfo{};
		paramsbufferInfo.buffer = uniformBuffer.buffer;
		paramsbufferInfo.offset = pipelineConstantsSize;
		paramsbufferInfo.range = parametersSize;

		vk::WriteDescriptorSet paramsBufferWrite;
		paramsBufferWrite.dstSet = parametersSet;
		// @Review
		paramsBufferWrite.dstBinding = 0; // binding 0 is reserved for material parameters 
		paramsBufferWrite.dstArrayElement = 0;
		paramsBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		paramsBufferWrite.descriptorCount = 1;
		paramsBufferWrite.pBufferInfo = &paramsbufferInfo;

		parametersdescriptorWrites.push_back(paramsBufferWrite);
	}
	
	for (auto const& [_, textureBinding] : paramsTextures)
	{
		vk::DescriptorImageInfo imageInfo;
		imageInfo.sampler = *textureBinding.texture->sampler;
		imageInfo.imageView = *textureBinding.texture->imageView;
		imageInfo.imageLayout = textureBinding.texture->image.getLayout();
		
		vk::WriteDescriptorSet textureWrite;
		textureWrite.dstSet = parametersSet;
		textureWrite.dstBinding = textureBinding.binding;
		textureWrite.dstArrayElement = 0;
		textureWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		textureWrite.descriptorCount = 1;
		textureWrite.pImageInfo = &imageInfo;
		
		parametersdescriptorWrites.push_back(textureWrite);
	}
	
	graphicsPipeline.deviceContext->device.updateDescriptorSets(std::size(parametersdescriptorWrites), parametersdescriptorWrites.data(), 0, nullptr);
}

void Material::setTextureParameter(std::string const& name, vkh::Texture& texture)
{
	auto const it = paramsTextures.find(name);
	if (it != paramsTextures.end())
	{
		it->second.texture = &texture;
		return;
	}
	throw std::runtime_error("texture not found !");
}

void Material::bind(vk::CommandBuffer cmd)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline.pipeline);
	vk::DescriptorSet sets[] = { /*constantsSet,*/ parametersSet};
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipeline.pipelineLayout, 0, std::size(sets), sets, 0, nullptr);
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

//vkh::ShaderVariable* Material::getConstant(std::string const& name)
//{
//	// @TODO handle this more elgantly;
//	auto& trueParams = constants[0].structType->members;
//	auto const it = std::find_if(trueParams.begin(), trueParams.end(), [&name](auto const& e)
//		{
//			return e.name == name;
//		});
//	if (it != trueParams.end())
//		return &(*it);
//	return nullptr;
//}

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
