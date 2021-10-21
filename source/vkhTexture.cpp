#include "vkhTexture.hpp"
#include "vkhBuffer.hpp"

using namespace vkh;

static vk::DeviceSize vkFormatToSize(vk::Format fmt)
{
	switch (fmt)
	{
	case vk::Format::eR8Unorm:
	case vk::Format::eR8Uint:
	case vk::Format::eR8Sint: 
	case vk::Format::eR8Srgb:
		return 1;
	case vk::Format::eR8G8Unorm: 
	case vk::Format::eR8G8Snorm:
	case vk::Format::eR8G8Uscaled:
	case vk::Format::eR8G8Sscaled: 
	case vk::Format::eR8G8Uint: 
	case vk::Format::eR8G8Sint: 
	case vk::Format::eR8G8Srgb:
		return 2;
	case vk::Format::eR8G8B8Unorm: 
	case vk::Format::eR8G8B8Snorm:
	case vk::Format::eR8G8B8Uscaled: 
	case vk::Format::eR8G8B8Sscaled: 
	case vk::Format::eR8G8B8Uint: 
	case vk::Format::eR8G8B8Sint:
	case vk::Format::eR8G8B8Srgb: 
	case vk::Format::eB8G8R8Unorm: 
	case vk::Format::eB8G8R8Snorm: 
	case vk::Format::eB8G8R8Uscaled: 
	case vk::Format::eB8G8R8Sscaled: 
	case vk::Format::eB8G8R8Uint:
	case vk::Format::eB8G8R8Sint: 
	case vk::Format::eB8G8R8Srgb:
		return 3;
	case vk::Format::eR8G8B8A8Unorm:
	case vk::Format::eR8G8B8A8Snorm:
	case vk::Format::eR8G8B8A8Uscaled: 
	case vk::Format::eR8G8B8A8Sscaled: 
	case vk::Format::eR8G8B8A8Uint: 
	case vk::Format::eR8G8B8A8Sint: 
	case vk::Format::eR8G8B8A8Srgb: 
	case vk::Format::eB8G8R8A8Unorm: 
	case vk::Format::eB8G8R8A8Snorm: 
	case vk::Format::eB8G8R8A8Uscaled: 
	case vk::Format::eB8G8R8A8Sscaled: 
	case vk::Format::eB8G8R8A8Uint: 
	case vk::Format::eB8G8R8A8Sint: 
	case vk::Format::eB8G8R8A8Srgb:
		return 4;
	case vk::Format::eR16Unorm:
	case vk::Format::eR16Snorm: 
	case vk::Format::eR16Uscaled: 
	case vk::Format::eR16Sscaled: 
	case vk::Format::eR16Uint:
	case vk::Format::eR16Sint: 
	case vk::Format::eR16Sfloat:
		return 2;
	case vk::Format::eR16G16Unorm: 
	case vk::Format::eR16G16Snorm:
	case vk::Format::eR16G16Uscaled:
	case vk::Format::eR16G16Sscaled: 
	case vk::Format::eR16G16Uint:
	case vk::Format::eR16G16Sint: 
	case vk::Format::eR16G16Sfloat:
		return 4;
	case vk::Format::eR16G16B16Unorm:
	case vk::Format::eR16G16B16Snorm: 
	case vk::Format::eR16G16B16Uscaled: 
	case vk::Format::eR16G16B16Sscaled:
	case vk::Format::eR16G16B16Uint:
	case vk::Format::eR16G16B16Sint: 
	case vk::Format::eR16G16B16Sfloat:
		return 6;
	case vk::Format::eR16G16B16A16Unorm: 
	case vk::Format::eR16G16B16A16Snorm: 
	case vk::Format::eR16G16B16A16Uscaled: 
	case vk::Format::eR16G16B16A16Sscaled:
	case vk::Format::eR16G16B16A16Uint:
	case vk::Format::eR16G16B16A16Sint: 
	case vk::Format::eR16G16B16A16Sfloat:
		return 8;
	case vk::Format::eR32Uint:
	case vk::Format::eR32Sint:
	case vk::Format::eR32Sfloat:
		return 4;
	case vk::Format::eR32G32Uint:
	case vk::Format::eR32G32Sint: 
	case vk::Format::eR32G32Sfloat: 
		return 8;
	case vk::Format::eR32G32B32Uint: 
	case vk::Format::eR32G32B32Sint:
	case vk::Format::eR32G32B32Sfloat:
		return 12;
	case vk::Format::eR32G32B32A32Uint:
	case vk::Format::eR32G32B32A32Sint: 
	case vk::Format::eR32G32B32A32Sfloat:
		return 16;
	case vk::Format::eR64Uint:
	case vk::Format::eR64Sint:
	case vk::Format::eR64Sfloat:
		return 8;
	case vk::Format::eR64G64Uint:
	case vk::Format::eR64G64Sint:
	case vk::Format::eR64G64Sfloat:
		return 16;
	case vk::Format::eR64G64B64Uint:
	case vk::Format::eR64G64B64Sint:
	case vk::Format::eR64G64B64Sfloat:
		return 24;
	case vk::Format::eR64G64B64A64Uint:
	case vk::Format::eR64G64B64A64Sint:
	case vk::Format::eR64G64B64A64Sfloat:
		return 32;
	default:
		throw std::runtime_error("unsuported size format");
	}
}

// @Improve: All textures are created with a staging buffer for now since most textures do not need to be modified by the CPU after creation
void Texture::create(DeviceContext& ctx, CreateInfo const& info)
{
	vk::DeviceSize const imageSize = info.width * info.height * vkFormatToSize(info.format);

	vk::BufferCreateInfo stagingBufferInfo;
	stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	stagingBufferInfo.size = imageSize;
	stagingBufferInfo.sharingMode = vk::SharingMode::eExclusive;
	
	vma::AllocationCreateInfo stagingBufferAllocInfo;
	stagingBufferAllocInfo.usage = vma::MemoryUsage::eCpuToGpu;
	
	vkh::Buffer stagingBuffer;
	stagingBuffer.create(ctx, stagingBufferInfo, stagingBufferAllocInfo);
	stagingBuffer.writeData(info.data);
	
	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled;
	imageInfo.extent = vk::Extent3D{ info.width, info.height, 1 };
	imageInfo.mipLevels = info.mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = info.format;
	imageInfo.tiling = info.tiling;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	
	vma::AllocationCreateInfo imageAllocInfo;
	imageAllocInfo.usage = vma::MemoryUsage::eGpuOnly;
	
	image.create(ctx, imageInfo, imageAllocInfo);
	image.transitionLayout(vk::ImageLayout::eTransferDstOptimal);

	stagingBuffer.copyToImage(image);
	
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = image.handle;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = imageInfo.format;
	
	// @Review
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = info.mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	imageView = ctx.device.createImageViewUnique(viewInfo, ctx.allocationCallbacks);

	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16.0f;
	samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(info.mipLevels);

	sampler = ctx.device.createSamplerUnique(samplerInfo, ctx.allocationCallbacks);
}

void Texture::destroy()
{
	image.destroy();
	imageView.reset();
	sampler.reset();
}

Texture::~Texture()
{
	destroy();
}


