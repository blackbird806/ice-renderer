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
		vk::Format swapchainFormat;
		vk::Extent2D swapchainExtent;
		std::vector<vk::Image> swapchainImages;
		std::vector<vk::UniqueImageView> swapchainImageViews;
	};
}
