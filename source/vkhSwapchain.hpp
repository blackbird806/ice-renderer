#pragma once
#include <vulkan/vulkan.hpp>

#include "vkhImage.hpp"

struct GLFWwindow;

namespace vkh
{
	struct Swapchain
	{
		void create(vkh::DeviceContext* ctx, GLFWwindow* window, vk::SurfaceKHR surface, uint minImages, bool vsync);
		void destroy();

		~Swapchain();
		
		vkh::DeviceContext* deviceContext;
		vk::UniqueSwapchainKHR swapchain;
		vk::Format format;
		vk::Extent2D extent;
		std::vector<vk::Image> images;
		std::vector<vk::UniqueImageView> imageViews;
	};
}
