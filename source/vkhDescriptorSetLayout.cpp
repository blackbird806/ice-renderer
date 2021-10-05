#include "vkhDescriptorSetLayout.hpp"

#include "vkhDeviceContext.hpp"
#include "vkhShader.hpp"

#include <ranges>

using namespace vkh;

void DescriptorSetLayout::create(vkh::DeviceContext& ctx, ShaderReflector const& vertData,
	ShaderReflector const& fragData)
{
	deviceContext = &ctx;
	
	auto const descriptorSetLayoutDatas = { vertData.getDescriptorSetLayoutData(), fragData.getDescriptorSetLayoutData() };
	
	for (auto const& e : std::ranges::join_view(descriptorSetLayoutDatas))
	{
		assert(e.set_number < MaxSets);
		
		layoutBindings[e.set_number].insert(layoutBindings[e.set_number].end(), e.bindings.begin(), e.bindings.end());
	}

	for (size_t i = 0; i < MaxSets; i++)
	{
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
		descriptorSetLayoutCreateInfo.bindingCount = std::size(layoutBindings[i]);
		descriptorSetLayoutCreateInfo.pBindings = layoutBindings[i].data();

		descriptorSetLayouts.push_back(
			ctx.device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo, ctx.allocationCallbacks));
	}
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
