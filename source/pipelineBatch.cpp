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

void PipelineBatch::addImageInfo(uint32 binding, vk::DescriptorImageInfo const& info)
{
	assert(binding < imageInfosArray.size());
	imageInfosArray[binding].push_back(info);
}

void PipelineBatch::setImageArraySize(size_t size)
{
	imageInfosArray.resize(size);
}

void PipelineBatch::updateTextureDescriptorSet()
{
	std::vector<vk::WriteDescriptorSet> descriptorWrites(imageInfosArray.size());
	
	for (int i = 0; i < imageInfosArray.size(); i++)
	{
		// @TODO
		imageInfosArray[i].resize(64, imageInfosArray[i][0]);

		descriptorWrites[i].dstSet = texturesSet;
		descriptorWrites[i].dstBinding = i;
		descriptorWrites[i].dstArrayElement = 0;
		descriptorWrites[i].descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrites[i].descriptorCount = imageInfosArray[i].size();
		descriptorWrites[i].pImageInfo = imageInfosArray[i].data();
	}
	
	pipeline->deviceContext->device.updateDescriptorSets(std::size(descriptorWrites), descriptorWrites.data(), 0, nullptr);
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
