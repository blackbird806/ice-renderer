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
		void createWithStaging(vkh::DeviceContext& ctx, vk::BufferCreateInfo bufferInfo, vma::AllocationCreateInfo const& allocInfo, std::span<uint8> data);
		
		void destroy();

		[[nodiscard]] void* map();
		void unmap();
		
		void writeData(std::span<uint8> data, size_t offset = 0);

		template<typename T>
		void writeStruct(T&& struct_, size_t offset = 0)
		{
			writeData({ reinterpret_cast<uint8*>(&struct_), sizeof(T) }, offset);
		}

		void copyToImage(vkh::Image& img);
		void copyToBuffer(vkh::Buffer& buf);
		
		~Buffer();
		
		vk::Buffer buffer;
		vma::Allocation allocation;
		vkh::DeviceContext* deviceContext;
		vk::DeviceSize size;
	};
}
