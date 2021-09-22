#include "vkhImage.hpp"

using namespace vkh;

void Image::create(vkh::DeviceContext* ctx, vk::ImageCreateInfo imageInfo_, vma::AllocationCreateInfo allocInfo)
{
	assert(ctx);
	deviceContext = ctx;
	imageInfo = imageInfo_;

	vk::Result res = deviceContext->gpuAllocator.createImage(&imageInfo, &allocInfo, &handle, &allocation, nullptr);
	if (res != vk::Result::eSuccess)
		throw std::runtime_error("failed to create image");
}

void Image::destroy()
{
	deviceContext->gpuAllocator.destroyImage(handle, allocation);
}

Image::~Image()
{
	destroy();
}
