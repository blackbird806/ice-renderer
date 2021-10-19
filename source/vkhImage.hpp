#pragma once

#include "vkhDeviceContext.hpp"

namespace vkh
{
	struct Image
	{
		Image() = default;
		Image(Image&& rhs) noexcept;
		Image& operator=(Image&& rhs) noexcept;
		
		void create(vkh::DeviceContext& ctx, vk::ImageCreateInfo const& imageInfo, vma::AllocationCreateInfo const& allocInfo);

		void destroy();

		vk::ImageLayout getLayout() const;
		
		void transitionLayout(vk::ImageLayout newLayout);
		
		~Image();
		
		vkh::DeviceContext* deviceContext;
		
		vk::Image handle;
		vma::Allocation allocation;
		vk::ImageCreateInfo imageInfo;
	};
}