#include "pipelineBatch.hpp"
#include "vkhDeviceContext.hpp"

void PipelineBatch::create(vkh::GraphicsPipeline& pipeline_, uint32 bufferCount)
{
	pipeline = &pipeline_;

	createDescriptorPool(bufferCount);
	pipelineConstantSets = pipeline_.createDescriptorSets(*descriptorPool, vk::ShaderStageFlagBits::eVertex, vkh::DescriptorSetIndex::PipelineConstants, bufferCount);
	pipelineMaterialsSets = pipeline_.createDescriptorSets(*descriptorPool, vk::ShaderStageFlagBits::eFragment, vkh::DescriptorSetIndex::Material, bufferCount);
	pipelineTexturesSets = pipeline_.createDescriptorSets(*descriptorPool, vk::ShaderStageFlagBits::eFragment, vkh::DescriptorSetIndex::Textures, bufferCount);

	// @TODO
	pipelineConstantBuffers.resize(bufferCount);
	vk::BufferCreateInfo pipelineConstantBufferInfo;
	pipelineConstantBufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	// @TODO
	// @Review alignement
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

void PipelineBatch::bindMaterial(vk::CommandBuffer cmdBuff, MaterialID_t matId, uint32 frameIndex)
{
	uint32 dynamicOffsets[] = { matId /* * sizeof(Material) */};
	cmdBuff.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline->pipelineLayout, 
		0, 1, &pipelineMaterialsSets[frameIndex], 1, dynamicOffsets);
}
