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
	void createDescriptorPool();

	void recreateSwapchain();
	
	[[nodiscard]] bool startFrame();

	void endFrame();
	
	vk::SampleCountFlagBits const msaaSamples = vk::SampleCountFlagBits::e4;
	uint const maxFramesInFlight = 2;
	uint currentFrame = 0;
	uint32 imageIndex = 0;
	bool vsync = false;
	bool resized = false;

	// @TODO FrameData instead ?
	
	vkh::DeviceContext deviceContext;
	vkh::Instance instance;
	vkh::Swapchain swapchain;
	vk::UniqueRenderPass defaultRenderPass;
	std::vector<vk::UniqueFramebuffer> framebuffers;
	vkh::GraphicsPipeline graphicsPipeline;
	vkh::CommandBuffers commandBuffers;
	vk::UniqueDescriptorPool descriptorPool;
	
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