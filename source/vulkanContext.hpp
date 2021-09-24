#pragma once

#include "ice.hpp"
#include "vkhCommandBuffers.hpp"
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
	void createMsResources();
	void createDepthResources();
	void createFramebuffers();
	void createSyncResources();
	
	vk::SampleCountFlagBits const msaaSamples = vk::SampleCountFlagBits::e1;
	uint const maxFramesInFlight = 2;
	bool vsync = false;
	
	vkh::DeviceContext deviceContext;
	vkh::Instance instance;
	vkh::Swapchain swapchain;
	vk::UniqueRenderPass defaultRenderPass;
	std::vector<vk::UniqueFramebuffer> framebuffers;
	vkh::GraphicsPipeline graphicsPipeline;
	vkh::CommandBuffers commandBuffers;

	std::vector<vk::UniqueSemaphore> imageAvailableSemaphores;
	std::vector<vk::UniqueSemaphore> renderFinishedSemaphores;

	std::vector<vk::UniqueFence> inFlightFences;
	
	vkh::Image msImage;
	vk::UniqueImageView msImageView;

	vkh::Image depthImage;
	vk::UniqueImageView depthImageView;
	
	GLFWwindow* window;
	vk::SurfaceKHR surface;
};