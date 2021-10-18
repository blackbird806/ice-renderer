#include "vkhGraphicsPipeline.hpp"

#include "vkhShader.hpp"
#include "vkhDeviceContext.hpp"
#include "vkhUtility.hpp"


vk::UniqueRenderPass vkh::createDefaultRenderPassMSAA(vkh::DeviceContext& deviceContext,
	vk::Format colorFormat, vk::SampleCountFlagBits msaaSamples)
{
	vk::AttachmentDescription colorAttachment = {};
	colorAttachment.format = colorFormat;
	colorAttachment.samples = msaaSamples;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eLoad;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = colorFormat;
	colorAttachmentResolve.samples = vk::SampleCountFlagBits::e1;
	colorAttachmentResolve.loadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachmentResolve.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachmentResolve.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachmentResolve.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachmentResolve.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachmentResolve.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat(deviceContext.physicalDevice);
	depthAttachment.samples = msaaSamples;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
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

	vk::AttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass = {};
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	vk::SubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
	dependency.srcAccessMask = vk::AccessFlagBits();
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
	dependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eColorAttachmentWrite;

	vk::AttachmentDescription attachments[] =
	{
		colorAttachment,
		depthAttachment,
		colorAttachmentResolve,
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

void vkh::GraphicsPipeline::create(vkh::DeviceContext& ctx, CreateInfo const& info)
{
	deviceContext = &ctx;
	
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	vk::Viewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(info.imageExtent.width);
	viewport.height = static_cast<float>(info.imageExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor = {};
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = info.imageExtent;

	vk::PipelineViewportStateCreateInfo viewportState = {};
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	vk::PipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = info.msaaSamples;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	vk::PipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = vk::CompareOp::eLess;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne; // Optional
	colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero; // Optional
	colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero; // Optional
	colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd; // Optional

	vk::PipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = vk::LogicOp::eCopy; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// @Review abstract pipeline layout + vertexInput (aka shader reflect infos) ?
	ShaderReflector const* shadersInfos[] = { &info.vertexShader.reflector, &info.fragmentShader.reflector };
	dsLayout.create(ctx, shadersInfos);

	std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
	descriptorSetLayouts.reserve(dsLayout.descriptorSetLayouts.size());

	for (auto const& [_, d] : dsLayout.descriptorSetLayouts)
		descriptorSetLayouts.push_back(*d);
	
	// Create pipeline layout
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.setLayoutCount = std::size(descriptorSetLayouts);
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	pipelineLayout = ctx.device.createPipelineLayoutUnique(pipelineLayoutInfo, ctx.allocationCallbacks);
	
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages(2);
	shaderStages[0] = info.vertexShader.getPipelineShaderStage();
	shaderStages[1] = info.fragmentShader.getPipelineShaderStage();

	auto [attributeDescriptions, bindingDescription] = info.vertexShader.reflector.getVertexDescriptions();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(std::size(attributeDescriptions));
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Create graphic pipeline
	vk::GraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.stageCount = std::size(shaderStages);
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.basePipelineHandle = nullptr; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	pipelineInfo.layout = *pipelineLayout;
	pipelineInfo.renderPass = info.renderPass;
	pipelineInfo.subpass = 0;

	pipeline = ctx.device.createGraphicsPipelineUnique(nullptr, { pipelineInfo }, ctx.allocationCallbacks);
}

std::vector<vk::DescriptorSet> vkh::GraphicsPipeline::createDescriptorSets(vk::DescriptorPool pool, vkh::DescriptorSetIndex setIndex, uint32 count)
{
	assert(setIndex < MaxSets);
	assert(count > 0);

	vk::DescriptorSetLayout const layout = *dsLayout.descriptorSetLayouts[setIndex];

	std::vector<vk::DescriptorSetLayout> layouts(count, layout);

	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = layouts.data();

	return deviceContext->device.allocateDescriptorSets(allocInfo);
}

void vkh::GraphicsPipeline::destroy()
{
	dsLayout.destroy();
	pipelineLayout.reset();
	pipeline.reset();
}
