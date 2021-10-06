#include "vkhBuffer.hpp"

using namespace vkh;

Buffer::Buffer(Buffer&& rhs) noexcept :
	buffer(rhs.buffer)
{
	rhs.buffer = vk::Buffer();
}

Buffer& Buffer::operator=(Buffer&& rhs) noexcept
{
	buffer = rhs.buffer;
	rhs.buffer = vk::Buffer();
	return *this;
}

// @TODO handle buffer creation with staging buffer
void Buffer::create(vma::Allocator gpuAlloc, vk::BufferCreateInfo const& bufferInfo, vma::AllocationCreateInfo const& allocInfo)
{
	gpuAllocator = gpuAlloc;
	
	vk::Result res = gpuAllocator.createBuffer(&bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
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
		gpuAllocator.destroyBuffer(buffer, allocation);
		buffer = vk::Buffer();
	}
}

void Buffer::writeData(std::span<uint8> data)
{
	assert(data.size_bytes() <= size);

	// @Review persistent mapped memory ?
	void* mapped = gpuAllocator.mapMemory(allocation);
		memcpy(mapped, data.data(), data.size_bytes());
	gpuAllocator.unmapMemory(allocation);
}

Buffer::~Buffer()
{
	destroy();
}

