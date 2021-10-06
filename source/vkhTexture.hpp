#pragma once

#include <vulkan/vulkan.hpp>
#include <span>
#include <vkhImage.hpp>

namespace vkh
{
	struct DeviceContext;
	
	struct Texture
	{
		struct CreateInfo
		{
			uint32 width, height;
			vk::Format format;
			vk::ImageTiling tiling;
			uint32 mipLevels;
			std::span<uint8> data;
		};
		
		void create(DeviceContext& ctx, CreateInfo const&);
		void destroy();

		Texture(Texture&& rhs) noexcept;
		Texture& operator=(Texture&& rhs) noexcept;
		
		// TODO
		vkh::Image image;
		vk::ImageView imageView;
		vk::Sampler sampler;
	};
}
