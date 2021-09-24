#include "vkhUtility.hpp"

vk::Format vkh::findSupportedFormat(vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, const vk::ImageTiling tiling, const vk::FormatFeatureFlags features)
{
	for (auto format : candidates)
	{
		vk::FormatProperties props = physicalDevice.getFormatProperties(format);;

		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}

		if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format");
}


vk::Format vkh::findDepthFormat(vk::PhysicalDevice physicalDevice)
{
	return vkh::findSupportedFormat(
		physicalDevice,
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}