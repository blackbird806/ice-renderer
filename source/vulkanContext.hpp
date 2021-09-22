#pragma once

#include "vkhInstance.hpp"
#include "vkhDeviceContext.hpp"

#include "ice.hpp"

struct GLFWwindow;

struct VulkanContext
{
	VulkanContext(GLFWwindow* win);
	~VulkanContext();
	
	void createSurface();
	
	vkh::DeviceContext deviceContext;
	vkh::Instance instance;
	
	GLFWwindow* window;
	vk::SurfaceKHR surface;
};