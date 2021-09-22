#pragma once

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.hpp>
#include <span>

#include "ice.hpp"

namespace vkh
{
	class Buffer
	{
		
	public:
		ICE_NON_DISPATCHABLE_CLASS(Buffer)
		
		void create(vma::Allocator gpuAlloc, vk::BufferCreateInfo bufferInfo, vma::AllocationCreateInfo allocInfo);
		
		void destroy();

		void writeData(std::span<uint8> data);
		
		~Buffer();
		
	private:
		
		vk::Buffer buffer;
		vma::Allocation allocation;
		vma::Allocator gpuAllocator;
	};
}
