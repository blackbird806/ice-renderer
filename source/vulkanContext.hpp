#pragma once

#include <vulkan/vulkan.hpp>
#define WIN32_LEAN_AND_MEAN
#include <GLFW/glfw3.h>
#undef max
#undef min

#include <vector>
#include <span>

// vulkan use depth 0 to 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vma/vk_mem_alloc.hpp>

#include "ice.hpp"
#include "vkhBuffer.hpp"

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 uv;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		vk::VertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = vk::VertexInputRate::eVertex;
		return bindingDescription;
	}

	static auto getAttributeDescriptions()
	{
		std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex, color);
		
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[2].offset = offsetof(Vertex, uv);

		return attributeDescriptions;
	}

	bool operator==(Vertex const& other) const
	{
		return pos == other.pos && uv == other.uv && color == other.color;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return (
				(hash<glm::vec3>()(vertex.pos) ^
					(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct VulkanContext
{
	static constexpr auto c_maxFramesInFlight = 2;
	
	std::vector<Vertex> vertices;
	std::vector<uint32> indices;
	
	bool vsync = false;
	bool isWireframe = false;
	
	GLFWwindow* window;
	
	vk::Instance instance;
	vk::AllocationCallbacks* allocator = nullptr;
	std::vector<vk::PhysicalDevice> physicalDevices;
	std::vector<vk::ExtensionProperties> extensions;
	std::vector<const char*> validationLayers;
	
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::Queue graphicsQueue;
	vk::Queue computeQueue;
	vk::Queue presentQueue;
	vk::Queue transferQueue;

	vma::Allocator mainAllocator;
	
	vk::SurfaceKHR surface;

	uint32 graphicsFamilyIndex;
	uint32 computeFamilyIndex;
	uint32 presentFamilyIndex;
	uint32 transferFamilyIndex;

	vk::Semaphore imageAvailableSemaphore[c_maxFramesInFlight];
	vk::Semaphore renderFinishedSemaphore[c_maxFramesInFlight];

	vk::Fence inFlightFences[c_maxFramesInFlight];

	vk::CommandPool commandPool;

	uint32 minImageCount;
	vk::SwapchainKHR swapchain;
	vk::Format swapchainFormat;
	vk::Extent2D swapchainExtent;
	std::vector<vk::Image> swapchainImages;
	std::vector<vk::UniqueImageView> swapchainImageViews;

	vk::DescriptorSetLayout descriptorSetLayout;
	vk::RenderPass renderPass;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;
	
	vk::Framebuffer framebuffers[c_maxFramesInFlight];

	std::vector<vk::CommandBuffer> commandBuffers;
	
	vk::Buffer vertexBuffer;
	vk::DeviceMemory vertexMemory;

	vk::Buffer indexBuffer;
	vk::DeviceMemory indexMemory;

	vk::Buffer uniformBuffer[c_maxFramesInFlight];
	vk::DeviceMemory uniformBufferMemory[c_maxFramesInFlight];

	vk::DescriptorPool descriptorPool;
	std::vector<vk::DescriptorSet> descriptorSets;

	vk::Image textureImage;
	vk::DeviceMemory textureImageMemory;
	vk::ImageView textureImageView;
	vk::Sampler textureSampler;
	uint32 miplevels;
	
	vk::Image depthImage;
	vk::DeviceMemory depthImageMemory;
	vk::ImageView depthImageView;

	vk::Image rtImage;
	vk::DeviceMemory rtImageMemory;
	vk::ImageView rtImageView;
	
	uint currentFrame = 0;
	bool resized = false;

	vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
	
	void loadModel();
	
	vk::CommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(vk::CommandBuffer cmdBuff);

	void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
	void copyBufferToImage(vk::Buffer src, vk::Image dst, uint32 width, uint32 height);
	void transitionImageLayout(vk::Image image, vk::Format format, uint32 miplevel, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
		vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory, uint32 miplevel, vk::SampleCountFlagBits numSample = vk::SampleCountFlagBits::e1);

	void createInstance(std::span<const char*> validationLayers);
	void createDevice(std::span<const char*> requiredExtensions);
	void createSurface();
	void createSyncPrimitives();
	void createCommandPool();
	void createSwapchain();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandBuffers();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createDepthResources();
	void createColorResources();

	void createTextureImage();
	void generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	void createTextureImageView();
	void createSampler();
	
	void recreateSwapchain();
	
	void recordCommandBuffers();

	void updateUniformBuffer(int32);
	void drawFrame();

	void destroyBuffers();
	void destroyTextureImage();
	void destroySyncPrimitives();
	void destroySwapchain();
	void destroy();
};

