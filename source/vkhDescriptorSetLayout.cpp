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
		layoutBindings.insert(layoutBindings.end(), e.bindings.begin(), e.bindings.end());
	}

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
	descriptorSetLayoutCreateInfo.bindingCount = std::size(layoutBindings);
	descriptorSetLayoutCreateInfo.pBindings = layoutBindings.data();

	descriptorSetLayouts.push_back(
		ctx.device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo, ctx.allocationCallbacks));
}

void DescriptorSetLayout::destroy()
{
	for (auto& descriptorLayout : descriptorSetLayouts)
	{
		deviceContext->device.destroyDescriptorSetLayout(descriptorLayout, deviceContext->allocationCallbacks);
	}
	layoutBindings.clear();
	descriptorSetLayouts.clear();
}
