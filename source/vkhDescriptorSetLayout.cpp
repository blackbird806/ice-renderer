#include "vkhDescriptorSetLayout.hpp"

#include "vkhDeviceContext.hpp"
#include "vkhShader.hpp"

using namespace vkh;

void ShaderDescriptorLayout::create(vkh::DeviceContext& ctx, ShaderReflector const& shaderInfos)
{
	deviceContext = &ctx;
	auto const dsLayoutData = shaderInfos.getDescriptorSetLayoutData();
	descriptorSetLayouts.reserve(dsLayoutData.size());
	for (auto const& layoutData : dsLayoutData)
	{
		descriptorSetLayouts.emplace(static_cast<DescriptorSetIndex>(layoutData.set_number),
			deviceContext->device.createDescriptorSetLayoutUnique(layoutData.create_info));
	}
}

void ShaderDescriptorLayout::destroy()
{
	descriptorSetLayouts.clear();
}
