#pragma once

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.hpp>
#include <span>
#include <type_traits>

#include "ice.hpp"

namespace vkh
{
	class Buffer
	{
		
	public:

		Buffer() = default;
		ICE_NON_DISPATCHABLE_CLASS(Buffer)
		
		void create(vma::Allocator gpuAlloc, vk::BufferCreateInfo bufferInfo, vma::AllocationCreateInfo allocInfo);
		
		void destroy();

		void writeData(std::span<uint8> data);

		template<typename T>
		void writeStruct(T&& struct_)
		{
			//static_assert(std::is_standard_layout_v<T>);
			writeData({ reinterpret_cast<uint8*>(&struct_), sizeof(T) });
		}
		
		~Buffer();
		
		vk::Buffer buffer;
		vma::Allocation allocation;
		vma::Allocator gpuAllocator;
		vk::DeviceSize size;
	};
}
