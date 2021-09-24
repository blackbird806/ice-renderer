#pragma once

#include "ice.hpp"
#include "vkhInstance.hpp"
#include "vkhDeviceContext.hpp"
#include "vkhSwapchain.hpp"
#include "vkhGraphicsPipeline.hpp"

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
	vkh::GrpahicsPipeline graphicsPipeline;
	
	GLFWwindow* window;
	vk::SurfaceKHR surface;
};