#include "material.hpp"
#include "vkhBuffer.hpp"
#include "vkhGraphicsPipeline.hpp"
#include "vkhDeviceContext.hpp"

void Material::setBuffer(vkh::Buffer const& buffer)
{
	vk::DescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = buffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;

	for (auto const& set : descriptorSets)
	{
		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = set;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorsWriteInfos.descriptorsWrite.push_back(descriptorWrite);
	}
	// used for lifetime only
	descriptorsWriteInfos.writeInfos.push_back(bufferInfo);
}

void Material::setBuffer(std::string const& name, vkh::Buffer const& buffer)
{
	
}

void Material::bind(vk::CommandBuffer cmdBuffer, uint32 index)
{
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipeline->pipelineLayout, 0, 1, &descriptorSets[index], 0, nullptr);
}

void Material::updateDescriptorSets()
{
	graphicsPipeline->deviceContext->device.updateDescriptorSets(std::size(descriptorsWriteInfos.descriptorsWrite), descriptorsWriteInfos.descriptorsWrite.data(), 0, nullptr);
	descriptorsWriteInfos.descriptorsWrite.clear();
	descriptorsWriteInfos.writeInfos.clear();
}
