#include "pipelineBatch.hpp"
#include "vkhDeviceContext.hpp"
#include "vkhTexture.hpp"

std::unordered_map<std::string, vkh::ShaderVariable> PipelineBatch::defaultPipelineConstants;

void PipelineBatch::create(vkh::GraphicsPipeline& pipeline_, vk::DescriptorPool pool)
{
	pipeline = &pipeline_;
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
	
	pipelineConstantsSet = pipeline_.createDescriptorSets(pool, vkh::PipelineConstants, 1)[0];
	texturesSet = pipeline_.createDescriptorSets(pool, vkh::Textures, 1)[0];

	// set arraysize depending of the shader params
	imageInfosArray.resize(pipeline_.dsLayout.reflectedDescriptors[vkh::Textures].bindings.size());

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

// @Review struct architecture
// @Performance cache results
void PipelineBatch::updatePipelineConstantBuffer()
{
	size_t offset = 0;
	for (auto const& binding : pipeline->dsLayout.reflectedDescriptors[vkh::DescriptorSetIndex::PipelineConstants].bindings)
	{
		for(auto const& mem : binding.element.structType->members)
		{
			auto const& it = defaultPipelineConstants.find(mem.name);
			if (it != defaultPipelineConstants.end() && it->second.type != vkh::ShaderVarType::ShaderStruct /*@TODO handle structs*/)
			{
				auto const size = it->second.getSize();
				// @TODO handle member alignement
				pipelineConstantBuffer.writeData({ (uint8*)it->second.arrayElements.data(), size }, offset);
				offset += size;
			}
		}
	}
}

int32 PipelineBatch::addTexture(uint32 binding, vkh::Texture const& text)
{
	assert(binding < imageInfosArray.size());

	vk::DescriptorImageInfo info;
	info.sampler = *text.sampler;
	info.imageView = *text.imageView;
	info.imageLayout = text.image.getLayout();
	
	imageInfosArray[binding].push_back(info);
	
	return imageInfosArray[binding].size() - 1;
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

	for (auto const& binding : pipeline->dsLayout.reflectedDescriptors[vkh::DescriptorSetIndex::PipelineConstants].bindings)
	{
		size += binding.element.getSize();
	}
	
	return size;
}
