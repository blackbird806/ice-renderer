#include "material.hpp"
#include "vkhBuffer.hpp"
#include "vkhGraphicsPipeline.hpp"
#include "vkhDeviceContext.hpp"

void Material::create(vkh::DeviceContext& deviceContext)
{
	vma::AllocationCreateInfo allocInfo;
	allocInfo.usage = vma::MemoryUsage::eCpuToGpu;
	
	vk::BufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	// @Review alignement
	bufferCreateInfo.size = getUniformBufferSize();
	bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	uniformBuffer.create(deviceContext.gpuAllocator, bufferCreateInfo, allocInfo);
}

size_t Material::getUniformBufferSize() const noexcept
{
	size_t bufferSize = 0;

	for (auto const& p : parameters)
		bufferSize += p.getSize();
	
	return bufferSize;
}

//@Review 
void Material::updateBuffer()
{
	void* bufferData = uniformBuffer.map();
	size_t offset = 0;
	for (auto const& p : parameters)
	{
		if ((p.typeFlags & SPV_REFLECT_TYPE_FLAG_ARRAY) || (p.typeFlags & SPV_REFLECT_TYPE_FLAG_STRUCT))
		{
			// array/structs inside materials are unsuported for now
			assert(false);
		}
		memcpy((uint8*)bufferData + offset, &p.value, p.getSize());
		offset += p.getSize();
	}
	uniformBuffer.unmap();
}

void Material::bind(vk::CommandBuffer cmdBuffer, uint32 index)
{
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipeline->pipelineLayout, 0, 1, &descriptorSets[index], 0, nullptr);
}

void Material::updateDescriptorSets()
{
	vk::DescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = uniformBuffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;

	std::vector<vk::WriteDescriptorSet> descriptorWrites;
	descriptorWrites.reserve(descriptorSets.size());
	
	for (auto const& set : descriptorSets)
	{
		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = set;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrites.push_back(descriptorWrite);
	}
	
	graphicsPipeline->deviceContext->device.updateDescriptorSets(std::size(descriptorWrites), descriptorWrites.data(), 0, nullptr);
}
