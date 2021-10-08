#include "vkhDescriptorSetLayout.hpp"

#include "vkhDeviceContext.hpp"
#include "vkhShader.hpp"

#include <ranges>
#include <utility.hpp>

using namespace vkh;

void DescriptorSetLayout::create(vkh::DeviceContext& ctx, ShaderReflector const& vertData,
	ShaderReflector const& fragData)
{
	deviceContext = &ctx;

	auto a = fragData.createDescriptorSetDescriptors();
	
	auto const descriptorSetLayoutDatas = { vertData.getDescriptorSetLayoutData(), fragData.getDescriptorSetLayoutData() };
	
	for (auto const& e : std::ranges::join_view(descriptorSetLayoutDatas))
	{
		assert(e.set_number < MaxSets);
		
		mergeVectors(layoutBindings[e.set_number], e.bindings);
	}

	for (size_t i = 0; i < MaxSets; i++)
	{
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
		descriptorSetLayoutCreateInfo.bindingCount = std::size(layoutBindings[i]);
		descriptorSetLayoutCreateInfo.pBindings = layoutBindings[i].data();

		descriptorSetLayouts.push_back(
			ctx.device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo, ctx.allocationCallbacks));
	}
	
	descriptorsDescriptors = vertData.createDescriptorSetDescriptors();
	mergeVectors(descriptorsDescriptors, fragData.createDescriptorSetDescriptors());
}

void DescriptorSetLayout::destroy()
{
	for (auto& descriptorLayout : descriptorSetLayouts)
	{
		deviceContext->device.destroyDescriptorSetLayout(descriptorLayout, deviceContext->allocationCallbacks);
	}
	
	for (auto& e : layoutBindings)
		e.clear();

	descriptorSetLayouts.clear();
}
