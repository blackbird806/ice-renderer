#pragma once

#include <functional>
#include <glm/glm.hpp>

#include "ice.hpp"
#include "vkhBuffer.hpp"
#include "vkhCommandBuffers.hpp"
#include "vkhInstance.hpp"
#include "vkhDeviceContext.hpp"
#include "vkhSwapchain.hpp"
#include "vkhGraphicsPipeline.hpp"

struct GLFWwindow;

struct UniformFrameConstants
{
	// use viewProj directly ?
	glm::mat4 view;
	glm::mat4 proj;

	float time;
};

struct VulkanContext
{
	VulkanContext(GLFWwindow* win);
	~VulkanContext();
	
	void createSurface();
	void createMsResources();
	void createDepthResources();
	std::vector<vk::UniqueFramebuffer> createPresentFramebuffers(vk::RenderPass presentPass);
	void createSyncResources();
	void createDescriptorPool();

	void destroyDepthResources();
	void destroyMsResources();
	
	void recreateSwapchain();
	
	[[nodiscard]] bool startFrame();

	void endFrame();

	std::function<void()> onSwapchainRecreate;
	
	vk::SampleCountFlagBits const msaaSamples = vk::SampleCountFlagBits::e4;
	uint const maxFramesInFlight = 2;
	uint currentFrame = 0;
	uint32 imageIndex = 0;
	bool vsync = false;
	bool resized = false;

	vkh::DeviceContext deviceContext;
	vkh::Instance instance;
	vkh::Swapchain swapchain;
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