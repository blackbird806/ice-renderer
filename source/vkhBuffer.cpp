#include "vkhBuffer.hpp"
#include "vkhImage.hpp"

#include "vkhCommandBuffers.hpp"

using namespace vkh;

Buffer::Buffer(Buffer&& rhs) noexcept :
	buffer(rhs.buffer)
{
	rhs.buffer = vk::Buffer();
}

Buffer& Buffer::operator=(Buffer&& rhs) noexcept
{
	destroy();
	buffer = rhs.buffer;
	rhs.buffer = vk::Buffer();
	return *this;
}

// @TODO handle buffer creation with staging buffer
void Buffer::create(vkh::DeviceContext& ctx, vk::BufferCreateInfo const& bufferInfo, vma::AllocationCreateInfo const& allocInfo)
{
	deviceContext = &ctx;
	
	vk::Result res = ctx.gpuAllocator.createBuffer(&bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
	size = bufferInfo.size;
	if (res != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create buffer");
	}
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

void Buffer::writeData(std::span<uint8> data)
{
	assert(data.size_bytes() <= size);

	void* mapped = map();
		memcpy(mapped, data.data(), data.size_bytes());
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

Buffer::~Buffer()
{
	destroy();
}

