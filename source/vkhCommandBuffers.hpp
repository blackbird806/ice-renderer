#pragma once

#include <vulkan/vulkan.hpp>
#include "ice.hpp"

namespace vkh
{
	struct DeviceContext;

	struct CommandBuffers
	{
		void create(vkh::DeviceContext& ctx, uint size);

		vk::CommandBuffer begin(uint index);

		void destroy();
		
		vk::CommandPool commandPool;
		std::vector<vk::UniqueCommandBuffer> commandBuffers;
	};

	struct SingleTimeCommandBuffer
	{
		SingleTimeCommandBuffer(vkh::DeviceContext* ctx);

		~SingleTimeCommandBuffer();
		
		vk::UniqueCommandBuffer cmdBuffer;
	};
}