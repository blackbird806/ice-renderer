#include "vkhDescriptorSetLayout.hpp"

#include "vkhDeviceContext.hpp"
#include "vkhShader.hpp"
#include "utility.hpp"

using namespace vkh;

void ShaderDescriptorLayout::create(vkh::DeviceContext& ctx, std::span<ShaderReflector const*> shadersInfos)
{
	deviceContext = &ctx;
	std::vector<ShaderReflector::DescriptorSetLayoutData> dsLayoutData;
	
	for (auto const& shaderInfo : shadersInfos)
	{
		for (auto& e : shaderInfo->getDescriptorSetLayoutData())
		{
			auto const it = std::find(dsLayoutData.begin(), dsLayoutData.end(), e);
			if (it != dsLayoutData.end()) // same set is found
			{
				// check for similar bindings
				for (auto& eb : e.bindings)
				{
					for (auto& ib : it->bindings)
					{
						// if similar binding update shaderstage flags
						if (eb.binding == ib.binding)
							ib.stageFlags |= eb.stageFlags;
					}
				}
			}
			else
				dsLayoutData.push_back(std::move(e));
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
