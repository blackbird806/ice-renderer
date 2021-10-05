#include "GUILayer.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include "vulkanContext.hpp"
#include "vkhUtility.hpp"

static vk::UniqueRenderPass createImguiRenderPass(vkh::DeviceContext& deviceContext,
	vk::Format colorFormat)
{
	vk::AttachmentDescription colorAttachment = {};
	colorAttachment.format = colorFormat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eLoad;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentDescription depthAttachment = {};
	depthAttachment.format = vkh::findDepthFormat(deviceContext.physicalDevice);
	depthAttachment.samples = vk::SampleCountFlagBits::e1;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
	depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::SubpassDescription subpass = {};
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	vk::SubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.srcAccessMask = vk::AccessFlagBits();
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

	vk::AttachmentDescription attachments[] =
	{
		colorAttachment,
		depthAttachment,
	};

	vk::RenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.attachmentCount = static_cast<uint32>(std::size(attachments));
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	return deviceContext.device.createRenderPassUnique(renderPassInfo, deviceContext.allocationCallbacks);
}

void GUILayer::init(VulkanContext& vkContext)
{
	auto const& device = vkContext.deviceContext.device;
	auto const& window = vkContext.window;

	// the size of the pool is very oversize, but it's copied from imgui demo itself.
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	imguiPool = device.createDescriptorPoolUnique(pool_info, vkContext.deviceContext.allocationCallbacks);

	//this initializes the core structures of imgui
	ImGui::CreateContext();

	// 2: initialize imgui library
	if (!ImGui_ImplGlfw_InitForVulkan(window, true))
		throw std::runtime_error("failed to initialise ImGui GLFW adapter");
	
	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = *vkContext.instance.handle;
	init_info.PhysicalDevice = vkContext.deviceContext.physicalDevice;
	init_info.Device = device;
	init_info.Queue = vkContext.deviceContext.graphicsQueue;
	init_info.DescriptorPool = *imguiPool;
	init_info.MinImageCount = vkContext.maxFramesInFlight;
	init_info.ImageCount = vkContext.maxFramesInFlight;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = reinterpret_cast<VkAllocationCallbacks const*>(vkContext.deviceContext.allocationCallbacks);

	renderPass = createImguiRenderPass(vkContext.deviceContext, vkContext.swapchain.format);

	createFramebuffers(vkContext);
	
	ImGui_ImplVulkan_Init(&init_info, *renderPass);

	//execute a gpu command to upload imgui font textures
	{
		vkh::SingleTimeCommandBuffer fontCmdBuff(vkContext.deviceContext);
		if (!ImGui_ImplVulkan_CreateFontsTexture((VkCommandBuffer)fontCmdBuff))
			throw std::runtime_error("failed to create ImGui font textures");
	}
	
	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void GUILayer::render(vk::CommandBuffer commandBuffer, uint index, vk::Extent2D extent)
{
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplVulkan_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::Render();

	auto& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	std::array<vk::ClearValue, 2> clearValues = {};
	clearValues[0].color = std::array{ 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.renderPass = *renderPass;
	renderPassInfo.framebuffer = *framebuffers[index];
	renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
	renderPassInfo.renderArea.extent = extent;
	renderPassInfo.clearValueCount = std::size(clearValues);
	renderPassInfo.pClearValues = clearValues.data();
	
	commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	commandBuffer.endRenderPass();
}

void GUILayer::handleSwapchainRecreation(VulkanContext& vkContext)
{
	renderPass = createImguiRenderPass(vkContext.deviceContext, vkContext.swapchain.format);
	destroyFrameBuffers();
	createFramebuffers(vkContext);
}

void GUILayer::destroy()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void GUILayer::createFramebuffers(VulkanContext& vkContext)
{
	for (int i = 0; i < vkContext.maxFramesInFlight; i++)
	{
		vk::ImageView attachments[] = {
			*vkContext.swapchain.imageViews[i],
			*vkContext.depthImageView,
		};

		vk::FramebufferCreateInfo framebufferInfo{};
		framebufferInfo.renderPass = *renderPass;
		framebufferInfo.attachmentCount = std::size(attachments);
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = vkContext.swapchain.extent.width;
		framebufferInfo.height = vkContext.swapchain.extent.height;
		framebufferInfo.layers = 1;

		framebuffers.push_back(
			vkContext.deviceContext.device.createFramebufferUnique(framebufferInfo, vkContext.deviceContext.allocationCallbacks));
	}
}

void GUILayer::destroyFrameBuffers()
{
	framebuffers.clear();
}
