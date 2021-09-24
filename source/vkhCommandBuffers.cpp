#include "vkhCommandBuffers.hpp"
#include "vkhDeviceContext.hpp"

using namespace vkh;

void CommandBuffers::create(vkh::DeviceContext& ctx, uint size)
{
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.commandPool = ctx.commandPool;
	allocInfo.commandBufferCount = size;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;

	commandBuffers = ctx.device.allocateCommandBuffersUnique(allocInfo);
}

vk::CommandBuffer CommandBuffers::begin(uint index)
{
	vk::CommandBufferBeginInfo beginInfo;
	commandBuffers[index]->begin(beginInfo);
	return *commandBuffers[index];
}

void CommandBuffers::destroy()
{
	commandBuffers.clear();
}
