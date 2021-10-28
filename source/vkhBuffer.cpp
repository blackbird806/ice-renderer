#include "vkhBuffer.hpp"
#include "vkhImage.hpp"

#include "vkhCommandBuffers.hpp"

using namespace vkh;

Buffer::Buffer(Buffer&& rhs) noexcept :
	buffer(rhs.buffer), allocation(rhs.allocation), deviceContext(rhs.deviceContext), size(rhs.size)
{
	rhs.buffer = vk::Buffer();
}

Buffer& Buffer::operator=(Buffer&& rhs) noexcept
{
	destroy();
	buffer = rhs.buffer;
	allocation = rhs.allocation;
	deviceContext = rhs.deviceContext;
	size = rhs.size;
	rhs.buffer = vk::Buffer();
	return *this;
}

void Buffer::create(vkh::DeviceContext& ctx, vk::BufferCreateInfo const& bufferInfo, vma::AllocationCreateInfo const& allocInfo)
{
	deviceContext = &ctx;

	vk::Result res = ctx.gpuAllocator.createBuffer(&bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
	size = bufferInfo.size;
	
	if (res != vk::Result::eSuccess)
		throw std::runtime_error("failed to create buffer");
}

void Buffer::createWithStaging(vkh::DeviceContext& ctx, vk::BufferCreateInfo bufferInfo,
	vma::AllocationCreateInfo const& allocInfo, std::span<uint8> data)
{
	deviceContext = &ctx;

	vk::BufferCreateInfo stagingBufferInfo;
	stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	stagingBufferInfo.size = bufferInfo.size;
	stagingBufferInfo.sharingMode = vk::SharingMode::eExclusive;

	vma::AllocationCreateInfo stagingBufferAllocInfo;
	stagingBufferAllocInfo.usage = vma::MemoryUsage::eCpuToGpu;

	vkh::Buffer stagingBuffer;
	stagingBuffer.create(ctx, stagingBufferInfo, stagingBufferAllocInfo);
	stagingBuffer.writeData(data);
	bufferInfo.usage |= vk::BufferUsageFlagBits::eTransferDst;

	vk::Result res = ctx.gpuAllocator.createBuffer(&bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
	size = bufferInfo.size;

	stagingBuffer.copyToBuffer(*this);
	
	if (res != vk::Result::eSuccess)
		throw std::runtime_error("failed to create buffer");
}

void Buffer::destroy()
{
	if (buffer)
	{
		deviceContext->gpuAllocator.destroyBuffer(buffer, allocation);
		buffer = vk::Buffer();
	}
}

	// @Review persistent mapped memory ?
void* Buffer::map()
{
	return deviceContext->gpuAllocator.mapMemory(allocation);
}

void Buffer::unmap()
{
	deviceContext->gpuAllocator.unmapMemory(allocation);
}

void Buffer::writeData(std::span<uint8> data, size_t offset)
{
	assert(data.size_bytes() + offset <= size);

	void* mapped = map();
		memcpy((uint8*)mapped + offset, data.data(), data.size_bytes());
	unmap();
}

void Buffer::copyToImage(vkh::Image& img)
{
	vkh::SingleTimeCommandBuffer cmd(*deviceContext);

	vk::BufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = img.imageInfo.arrayLayers;

	region.imageOffset = vk::Offset3D{ 0, 0, 0 };
	region.imageExtent = img.imageInfo.extent;

	cmd->copyBufferToImage(buffer, img.handle, vk::ImageLayout::eTransferDstOptimal, { region });
}

void Buffer::copyToBuffer(vkh::Buffer& buf)
{
	vkh::SingleTimeCommandBuffer cmd(*deviceContext);
	vk::BufferCopy copyRegion{};
	copyRegion.size = size;
	cmd->copyBuffer(buffer, buf.buffer, { copyRegion });
}

Buffer::~Buffer()
{
	destroy();
}

