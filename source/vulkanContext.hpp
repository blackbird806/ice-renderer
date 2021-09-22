#pragma once

#include "vkhInstance.hpp"
#include "vkhDeviceContext.hpp"

#include "ice.hpp"
#include "vkhImage.hpp"

struct GLFWwindow;

struct VulkanContext
{
	VulkanContext(GLFWwindow* win);
	~VulkanContext();
	
	void createSurface();
	void createSwapchain();

	uint const maxFramesInFlight = 2;
	bool vsync = false;
	
	vkh::DeviceContext deviceContext;
	vkh::Instance instance;

	
	GLFWwindow* window;
	vk::SurfaceKHR surface;
};