#pragma once

#include <vulkan/vulkan.hpp>

#include <vma/vk_mem_alloc.hpp>

namespace vkh
{
	struct DeviceContext
	{
		vk::Device device;
		vk::PhysicalDevice physicalDevice;
		vk::AllocationCallbacks allocationCallbacks;

		vma::Allocator gpuAllocator;

		vk::Queue graphicsQueue;
		vk::Queue presentQueue;
	};
}