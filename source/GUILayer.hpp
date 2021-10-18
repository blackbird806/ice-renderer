#pragma once
#include <vulkan/vulkan.hpp>

#include "ice.hpp"

struct VulkanContext;

class GUILayer
{
public:
	void init(VulkanContext& vkContext);

	void startFrame();
	void render(vk::CommandBuffer commandBuffer, uint index, vk::Extent2D extent);
	void handleSwapchainRecreation(VulkanContext& vkContext);
	
	void destroy();
private:

	void createFramebuffers(VulkanContext&);
	void destroyFrameBuffers();
	
	vk::UniqueDescriptorPool imguiPool;
	std::vector<vk::UniqueFramebuffer> framebuffers;
	vk::UniqueRenderPass renderPass;
};
