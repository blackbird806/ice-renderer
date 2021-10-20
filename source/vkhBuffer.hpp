#pragma once

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.hpp>
#include <span>

#include "ice.hpp"

namespace vkh
{
	struct DeviceContext;
	struct Image;

	class Buffer
	{
		
	public:

		Buffer() = default;
		Buffer(Buffer&&) noexcept;

		Buffer& operator=(Buffer&&) noexcept;
		
		ICE_NON_COPYABLE_CLASS(Buffer)
		
		void create(vkh::DeviceContext& ctx, vk::BufferCreateInfo const& bufferInfo, vma::AllocationCreateInfo const& allocInfo);
		
		void destroy();

		[[nodiscard]] void* map();
		void unmap();
		
		void writeData(std::span<uint8> data);

		template<typename T>
		void writeStruct(T&& struct_)
		{
			writeData({ reinterpret_cast<uint8*>(&struct_), sizeof(T) });
		}

		void copyToImage(vkh::Image& img);
		
		~Buffer();
		
		vk::Buffer buffer;
		vma::Allocation allocation;
		vkh::DeviceContext* deviceContext;
		vk::DeviceSize size;
	};
}
