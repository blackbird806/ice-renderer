#pragma once

#include "vkhDeviceContext.hpp"

namespace vkh
{
	struct Image
	{
		void create(vkh::DeviceContext& ctx, vk::ImageCreateInfo imageInfo, vma::AllocationCreateInfo allocInfo);
		void destroy();

		~Image();
		
		vkh::DeviceContext* deviceContext;
		
		vk::Image handle;
		vma::Allocation allocation;
		vk::ImageCreateInfo imageInfo;
	};
}