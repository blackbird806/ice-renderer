#pragma once

#include <vulkan/vulkan.hpp>
#include <vkhImage.hpp>

namespace vkh
{
	struct Texture
	{
		// TODO
		vkh::Image image;
		vk::ImageView imageView;
		vk::Sampler sampler;
	};
}
