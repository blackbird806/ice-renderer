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

SingleTimeCommandBuffer::SingleTimeCommandBuffer(vkh::DeviceContext& ctx) : deviceContext(ctx)
{
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.commandPool = ctx.commandPool;
	allocInfo.commandBufferCount = 1;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	cmdBuffer = std::move(ctx.device.allocateCommandBuffersUnique(allocInfo)[0]);

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	cmdBuffer->begin(beginInfo);
}

SingleTimeCommandBuffer::~SingleTimeCommandBuffer()
{
	end();
}

void SingleTimeCommandBuffer::end()
{
	cmdBuffer->end();
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer.get();
	deviceContext.graphicsQueue.submit(submitInfo, vk::Fence{});
	deviceContext.graphicsQueue.waitIdle();
}
