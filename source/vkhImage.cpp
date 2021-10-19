#include "vkhImage.hpp"
#include "vkhCommandBuffers.hpp"
#include "vkhUtility.hpp"

using namespace vkh;

Image::Image(Image&& rhs) noexcept : deviceContext(rhs.deviceContext), handle(rhs.handle),
	allocation(rhs.allocation), imageInfo(rhs.imageInfo)
{
	rhs.handle = vk::Image();
}

Image& Image::operator=(Image&& rhs) noexcept
{
	destroy();
	deviceContext = rhs.deviceContext;
	imageInfo = rhs.imageInfo;
	handle = rhs.handle;
	allocation = rhs.allocation;
	rhs.handle = vk::Image();
	
	return *this;
}

void Image::create(vkh::DeviceContext& ctx, vk::ImageCreateInfo const& imageInfo_, vma::AllocationCreateInfo const& allocInfo)
{
	deviceContext = &ctx;
	imageInfo = imageInfo_;

	vk::Result res = deviceContext->gpuAllocator.createImage(&imageInfo, &allocInfo, &handle, &allocation, nullptr);
	if (res != vk::Result::eSuccess)
		throw std::runtime_error("failed to create image");
}

void Image::destroy()
{
	if (handle)
	{
		deviceContext->gpuAllocator.destroyImage(handle, allocation);
		handle = vk::Image();
	}
}

vk::ImageLayout Image::getLayout() const
{
	return imageInfo.initialLayout;
}

void Image::transitionLayout(vk::ImageLayout newLayout)
{
	vk::ImageMemoryBarrier barrier{};
	barrier.oldLayout = getLayout();
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = handle;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = imageInfo.mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

		if (hasStencilComponent(imageInfo.format))
		{
			barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	}

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (getLayout() == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlags();
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (getLayout() == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (getLayout() == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlags();
		barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else
	{
		throw std::runtime_error("unsupported layout transition");
	}
	
	vkh::SingleTimeCommandBuffer cmd(*deviceContext);
	cmd->pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
	// update current layout
	imageInfo.initialLayout = newLayout;
}

Image::~Image()
{
	destroy();
}
