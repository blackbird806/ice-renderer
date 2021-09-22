#include "vkhSwapchain.hpp"

#include <GLFW/glfw3.h>
#undef min
#undef max

using namespace vkh;

static auto querySwapchainSupport(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
{
	struct SupportDetails
	{
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	SupportDetails const details{
		.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface),
		.formats = physicalDevice.getSurfaceFormatsKHR(surface),
		.presentModes = physicalDevice.getSurfacePresentModesKHR(surface)
	};
	return details;
}

static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
{
	if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
	{
		return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	}

	for (const auto& format : formats)
	{
		if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return format;
		}
	}

	throw std::runtime_error("found no suitable surface format");
}

static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& presentModes, bool vsync)
{
	// VK_PRESENT_MODE_IMMEDIATE_KHR: 
	//   Images submitted by your application are transferred to the screen right away, which may result in tearing.
	// VK_PRESENT_MODE_FIFO_KHR: 
	//   The swap chain is a queue where the display takes an image from the front of the queue when the display is
	//   refreshed and the program inserts rendered images at the back of the queue. If the queue is full then the program 
	//   has to wait. This is most similar to vertical sync as found in modern games. The moment that the display is 
	//   refreshed is known as "vertical blank".
	// VK_PRESENT_MODE_FIFO_RELAXED_KHR:
	//   This mode only differs from the previous one if the application is late and the queue was empty at the last 
	//   vertical blank. Instead of waiting for the next vertical blank, the image is transferred right away when it 
	//   finally arrives. This may result in visible tearing.
	// VK_PRESENT_MODE_MAILBOX_KHR: 
	//   This is another variation of the second mode. Instead of blocking the application when the queue is full, the 
	//   images that are already queued are simply replaced with the newer ones.This mode can be used to implement triple 
	//   buffering, which allows you to avoid tearing with significantly less latency issues than standard vertical sync 
	//   that uses double buffering.

	if (vsync)
	{
		return vk::PresentModeKHR::eFifo;
	}

	if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eMailbox) != presentModes.end())
	{
		return vk::PresentModeKHR::eMailbox;
	}

	if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eImmediate) != presentModes.end())
	{
		return vk::PresentModeKHR::eImmediate;
	}

	if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eFifoRelaxed) != presentModes.end())
	{
		return vk::PresentModeKHR::eFifoRelaxed;
	}

	return vk::PresentModeKHR::eFifo;
}

static vk::Extent2D chooseSwapExtent(GLFWwindow* window, const vk::SurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32>::max())
		return capabilities.currentExtent;

	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	vk::Extent2D actualExtent = { static_cast<uint32>(w), static_cast<uint32>(h) };

	actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
	actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

	return actualExtent;
}

void Swapchain::create(vkh::DeviceContext* ctx, GLFWwindow* window, vk::SurfaceKHR surface, uint minImages, bool vsync)
{
	assert(ctx);
	deviceContext = ctx;

	auto const details = querySwapchainSupport(deviceContext->physicalDevice, surface);

	if (details.formats.empty() || details.presentModes.empty())
		throw std::runtime_error("empty swap chain support");

	auto const surfaceFormat = chooseSwapSurfaceFormat(details.formats);
	auto const presentMode = chooseSwapPresentMode(details.presentModes, vsync);
	swapchainExtent = chooseSwapExtent(window, details.capabilities);

	auto const device = deviceContext->device;
	
	vk::SwapchainCreateInfoKHR createInfo = {};
	createInfo.surface = surface;
	createInfo.minImageCount = minImages;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapchainExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
	createInfo.preTransform = details.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = vk::PresentModeKHR::eFifo;
	createInfo.clipped = VK_TRUE;

	if (deviceContext->graphicsFamilyIndex != deviceContext->presentFamilyIndex)
	{
		uint32_t queueFamilyIndices[] = { deviceContext->graphicsFamilyIndex, deviceContext->presentFamilyIndex };

		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	swapchain = device.createSwapchainKHRUnique(createInfo, deviceContext->allocationCallbacks);

	//minImages = details.capabilities.minImageCount;
	swapchainFormat = surfaceFormat.format;
	swapchainImages = device.getSwapchainImagesKHR(*swapchain);

	for (auto const& image : swapchainImages)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imageViewCreateInfo.format = swapchainFormat;
		imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		swapchainImageViews.push_back(device.createImageViewUnique(imageViewCreateInfo, deviceContext->allocationCallbacks));
	}
}

void Swapchain::destroy()
{
	swapchain.reset();
	swapchainImageViews.clear();
}

Swapchain::~Swapchain()
{
	destroy();
}
