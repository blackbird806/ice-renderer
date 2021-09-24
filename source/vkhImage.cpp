#include "vkhImage.hpp"

using namespace vkh;

void Image::create(vkh::DeviceContext& ctx, vk::ImageCreateInfo imageInfo_, vma::AllocationCreateInfo allocInfo)
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

Image::~Image()
{
	destroy();
}
