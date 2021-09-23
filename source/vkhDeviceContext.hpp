#pragma once

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.hpp>

#include "ice.hpp"
#include "vkhInstance.hpp"

namespace vkh
{
	struct DeviceContext
	{
		void create(vkh::Instance const& instance, vk::SurfaceKHR const& surface_, std::span<const char*> requiredExtensions_);
		void destroy();
		void checkRequiredExtensions(vk::PhysicalDevice physicalDevice) const;
		
		vk::Device device;
		vk::PhysicalDevice physicalDevice;
		vk::AllocationCallbacks* allocationCallbacks;
		std::vector<const char*> requiredExtensions;

		vk::CommandPool commandPool;
		
		vma::Allocator gpuAllocator;

		uint32 graphicsFamilyIndex;
		uint32 computeFamilyIndex;
		uint32 presentFamilyIndex;
		uint32 transferFamilyIndex;
		
		vk::Queue graphicsQueue;
		vk::Queue presentQueue;
		vk::Queue transferQueue;
		vk::Queue computeQueue;
	};
}