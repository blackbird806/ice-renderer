#pragma once
#include <vulkan/vulkan.hpp>

namespace vkh
{
	vk::Format findSupportedFormat(vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, const vk::ImageTiling tiling, const vk::FormatFeatureFlags features);

	vk::Format findDepthFormat(vk::PhysicalDevice physicalDevice);

}
