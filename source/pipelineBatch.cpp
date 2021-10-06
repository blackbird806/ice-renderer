#include "pipelineBatch.hpp"
#include "vkhDeviceContext.hpp"

void PipelineBatch::create(vkh::GraphicsPipeline& pipeline_, uint32 bufferCount)
{
	pipeline = &pipeline_;

	createDescriptorPool(bufferCount);
	pipelineConstantSets = pipeline_.createDescriptorSets(*descriptorPool, vkh::DescriptorSetLayout::SetIndex::FrameConstants, bufferCount);
	pipelineMaterialsSets = pipeline_.createDescriptorSets(*descriptorPool, vkh::DescriptorSetLayout::SetIndex::Material, bufferCount);
	pipelineTexturesSets = pipeline_.createDescriptorSets(*descriptorPool, vkh::DescriptorSetLayout::SetIndex::Textures, bufferCount);

	// @TODO
	pipelineConstantBuffers.resize(bufferCount);
	vk::BufferCreateInfo pipelineConstantBufferInfo;
	pipelineConstantBufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	// @TODO
	//pipelineConstantBufferInfo.size = dsLayoutSize;
	pipelineConstantBufferInfo.sharingMode = vk::SharingMode::eExclusive;

	// @TODO
	vma::AllocationCreateInfo allocInfo;
	allocInfo.usage = vma::MemoryUsage::eCpuToGpu;
	
	for (auto& buffer : pipelineConstantBuffers)
		buffer.create(pipeline_.deviceContext->gpuAllocator, pipelineConstantBufferInfo, allocInfo);
}

void PipelineBatch::destroy()
{
	// destroy the pool will free descriptor sets
	destroyDescriptorPool();
}

void PipelineBatch::createDescriptorPool(uint32 bufferCount)
{
	// @Review PoolSize
	vk::DescriptorPoolSize poolSize[2];
	poolSize[0].type = vk::DescriptorType::eUniformBuffer;
	poolSize[0].descriptorCount = bufferCount;
	poolSize[1].type = vk::DescriptorType::eCombinedImageSampler;
	poolSize[1].descriptorCount = bufferCount;

	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.poolSizeCount = std::size(poolSize);
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = bufferCount;

	descriptorPool = pipeline->deviceContext->device.createDescriptorPoolUnique(poolInfo, pipeline->deviceContext->allocationCallbacks);
}

void PipelineBatch::destroyDescriptorPool()
{
	descriptorPool.reset();
}

void PipelineBatch::bind(vk::CommandBuffer cmdBuff, uint32 frameIndex)
{
	cmdBuff.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline->pipeline);
	cmdBuff.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline->pipelineLayout, 0, 1, &pipelineConstantSets[frameIndex], 0, nullptr);
	for (uint32 i = 0; auto const& mtrl : materials)
	{
		// @TODO
		uint32 materialOffset = i; // * sizeof(MaterialBuffer)
		cmdBuff.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline->pipelineLayout, 0, 1, &pipelineMaterialsSets[frameIndex], 0, &materialOffset);
		i++;
	}
}
