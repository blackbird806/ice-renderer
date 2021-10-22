#include "pipelineBatch.hpp"

void PipelineBatch::create(vkh::GraphicsPipeline& pipeline_, vk::DescriptorPool pool, uint32 batchSize_)
{
	assert(batchSize > 0);
	
	pipeline = &pipeline_;
	batchSize = batchSize_;
	auto& deviceContext = *pipeline_.deviceContext;

	{
		vma::AllocationCreateInfo allocInfo;
		allocInfo.usage = vma::MemoryUsage::eCpuToGpu;
		vk::BufferCreateInfo bufferCreateInfo;
		bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		// @Review alignement
		bufferCreateInfo.size = getPipelineConstantsBufferEntrySize();
		bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		pipelineConstantBuffer.create(deviceContext, bufferCreateInfo, allocInfo);
	}
	
	// @Review nb sets
	pipelineConstantsSet = pipeline_.createDescriptorSets(pool, vkh::PipelineConstants, 1)[0];
	texturesSet = pipeline_.createDescriptorSets(pool, vkh::Textures, 1)[0];
	
	updatePipelineConstantsSet();
}

void PipelineBatch::updatePipelineConstantsSet() const
{
	vk::DescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = pipelineConstantBuffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;

	vk::WriteDescriptorSet descriptorWrites[1];
	descriptorWrites[0].dstSet = pipelineConstantsSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	pipeline->deviceContext->device.updateDescriptorSets(std::size(descriptorWrites), descriptorWrites, 0, nullptr);
}

void PipelineBatch::addImageInfo(vk::DescriptorImageInfo const& info)
{
	imageInfos.push_back(info);
}

void PipelineBatch::updateTextureDescriptorSet()
{
	// @TODO
	imageInfos.resize(64, imageInfos[0]);
	
	vk::WriteDescriptorSet descriptorWrites[1];
	descriptorWrites[0].dstSet = texturesSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descriptorWrites[0].descriptorCount = imageInfos.size();
	descriptorWrites[0].pImageInfo = imageInfos.data();

	pipeline->deviceContext->device.updateDescriptorSets(std::size(descriptorWrites), descriptorWrites, 0, nullptr);
}

vk::DeviceSize PipelineBatch::getPipelineConstantsBufferEntrySize() const
{
	vk::DeviceSize size = 0;
	for (auto const& reflectedDescriptor : pipeline->dsLayout.reflectedDescriptors)
	{
		if (reflectedDescriptor.setNumber == vkh::DescriptorSetIndex::PipelineConstants)
		{
			for (auto const& binding : reflectedDescriptor.bindings)
			{
				size += binding.element.getSize();
			}
		}
	}
	return size;
}
