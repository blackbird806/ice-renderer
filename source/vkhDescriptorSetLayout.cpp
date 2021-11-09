#include "vkhDescriptorSetLayout.hpp"

#include "vkhDeviceContext.hpp"
#include "vkhShader.hpp"
#include "utility.hpp"

using namespace vkh;

void ShaderDescriptorLayout::create(vkh::DeviceContext& ctx, std::span<ShaderReflector const*> shadersInfos)
{
	deviceContext = &ctx;
	std::vector<ShaderReflector::DescriptorSetLayoutData> dsLayoutData;

	// correctly merge shaders descriptor sets
	// TODO handle different declaration of same set
	for (auto const& shaderInfo : shadersInfos)
	{
		for (auto& e : shaderInfo->getDescriptorSetLayoutData())
		{
			insertUnique(dsLayoutData, std::move(e));
		}

		for (auto& reflectedSet : shaderInfo->createReflectedDescriptorSet())
		{
			insertUnique(reflectedDescriptors, std::move(reflectedSet));
		}
	}

	std::sort(reflectedDescriptors.begin(), reflectedDescriptors.end(), [](auto& a, auto& b)
		{
			return a.setNumber < b.setNumber;
		});
	
	// set correct shader stages to bindings
	// @Review this seems to set more flags than needed
	for (auto& dsLayout : dsLayoutData)
	{
		for(auto& binding : dsLayout.bindings)
		{
			for (auto const& shaderInfo : shadersInfos)
			{
				binding.stageFlags |= shaderInfo->getShaderStage();
			}
		}
	}
	
	descriptorSetLayouts.reserve(dsLayoutData.size());
	for (auto& layoutData : dsLayoutData)
	{
		layoutData.create_info.pBindings = layoutData.bindings.data();
		layoutData.create_info.bindingCount = layoutData.bindings.size();
		
		descriptorSetLayouts.emplace(static_cast<DescriptorSetIndex>(layoutData.set_number),
			deviceContext->device.createDescriptorSetLayoutUnique(layoutData.create_info));
	}
}

void ShaderDescriptorLayout::destroy()
{
	descriptorSetLayouts.clear();
}
