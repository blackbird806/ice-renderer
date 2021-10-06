#pragma once

#include "vkhDeviceContext.hpp"

namespace vkh
{
	struct Image
	{
		void create(vkh::DeviceContext& ctx, vk::ImageCreateInfo imageInfo, vma::AllocationCreateInfo allocInfo);
		void destroy();

		void transitionLayout(vk::Format format, uint32 miplevel, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
		
		~Image();
		
		vkh::DeviceContext* deviceContext;
		
		vk::Image handle;
		vma::Allocation allocation;
		vk::ImageCreateInfo imageInfo;
	};
}