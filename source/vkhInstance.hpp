#pragma once

#include <span>
#include <vulkan/vulkan.hpp>

namespace vkh
{
	struct Instance
	{
		void create(const char* appName, const char* engineName, std::span<const char*> validationLayers_, vk::AllocationCallbacks* alloc);
		void destroy();
		
		bool validationLayersEnabled() const noexcept;
		
		vk::UniqueInstance handle;
		vk::AllocationCallbacks* allocationCallbacks;
		std::vector<vk::PhysicalDevice> physicalDevices;
		std::vector<vk::ExtensionProperties> extensions;
		std::vector<const char*> validationLayers;

	private:
		bool checkValidationLayerSupport() const;
	};
}
