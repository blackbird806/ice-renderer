#pragma once

#include "vkhInstance.hpp"
#include "vkhDeviceContext.hpp"

#include "ice.hpp"
#include "vkhSwapchain.hpp"

struct GLFWwindow;

struct VulkanContext
{
	VulkanContext(GLFWwindow* win);
	~VulkanContext();
	
	void createSurface();

	uint const maxFramesInFlight = 2;
	bool vsync = false;
	
	vkh::DeviceContext deviceContext;
	vkh::Instance instance;
	vkh::Swapchain swapchain;
	
	GLFWwindow* window;
	vk::SurfaceKHR surface;
};