#include "vkhGraphicsPipeline.hpp"

#include <iterator>
#include <ranges>

#include "vkhShader.hpp"
#include "vkhDeviceContext.hpp"
#include "vkhUtility.hpp"

vk::UniqueRenderPass vkh::createDefaultRenderPass(vkh::DeviceContext& deviceContext,
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

void vkh::GraphicsPipeline::create(vkh::DeviceContext& deviceContext, std::span<uint8> vertSpv,
		std::span<uint8> fragSpv, vk::RenderPass renderPass, vk::Extent2D imageExtent, vk::SampleCountFlagBits msaaSamples)
{
	
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	vk::Viewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(imageExtent.width);
	viewport.height = static_cast<float>(imageExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor = {};
	scissor.offset = vk::Offset2D{ 0, 0 };
	scissor.extent = imageExtent;

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
	multisampling.rasterizationSamples = msaaSamples;
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

	// Create descriptor pool/sets.

	ShaderReflector vertReflector(vertSpv);
	ShaderReflector fragReflector(fragSpv);

	auto const descriptorSetLayoutDataVert = vertReflector.getDescriptorSetLayoutData();
	auto const descriptorSetLayoutDataFrag = fragReflector.getDescriptorSetLayoutData();

	// @Review descriptor set layouts merge
	auto const descriptorSetLayoutDatas = { descriptorSetLayoutDataVert, descriptorSetLayoutDataFrag };
	std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
	for (auto const& e : std::ranges::join_view(descriptorSetLayoutDatas))
	{
		layoutBindings.insert(layoutBindings.end(), e.bindings.begin(), e.bindings.end());
	}
	
	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
	descriptorSetLayoutCreateInfo.bindingCount = std::size(layoutBindings);
	descriptorSetLayoutCreateInfo.pBindings = layoutBindings.data();

	descriptorSetLayouts.push_back(
		deviceContext.device.createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo, deviceContext.allocationCallbacks));

	// Create pipeline layout and render pass.

	std::vector<vk::DescriptorSetLayout> pSetLayouts(descriptorSetLayouts.size());
	for (int i = 0; auto& layout : pSetLayouts)
		layout = *descriptorSetLayouts[i++];

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.setLayoutCount = std::size(pSetLayouts);
	pipelineLayoutInfo.pSetLayouts = pSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	pipelineLayout = deviceContext.device.createPipelineLayoutUnique(pipelineLayoutInfo, deviceContext.allocationCallbacks);

	vk::ShaderModuleCreateInfo vertexShaderCreateInfo{};
	vertexShaderCreateInfo.codeSize = vertSpv.size();
	vertexShaderCreateInfo.pCode = reinterpret_cast<uint32_t const*>(vertSpv.data());

	vk::UniqueShaderModule vertexShader = deviceContext.device.createShaderModuleUnique(vertexShaderCreateInfo, deviceContext.allocationCallbacks);

	vk::ShaderModuleCreateInfo fragmentShaderCreateInfo{};
	fragmentShaderCreateInfo.codeSize = fragSpv.size();
	fragmentShaderCreateInfo.pCode = reinterpret_cast<uint32_t const*>(fragSpv.data());

	vk::UniqueShaderModule fragmentShader = deviceContext.device.createShaderModuleUnique(fragmentShaderCreateInfo, deviceContext.allocationCallbacks);

	vk::PipelineShaderStageCreateInfo vertexStageInfo{};
	vertexStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertexStageInfo.module = vertexShader.get();
	vertexStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragmentStageInfo{};
	fragmentStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragmentStageInfo.module = fragmentShader.get();
	fragmentStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo shaderStages[] =
	{
		vertexStageInfo,
		fragmentStageInfo
	};

	auto [attributeDescriptions, bindingDescription] = vertReflector.getVertexDescriptions();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(std::size(attributeDescriptions));
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Create graphic pipeline
	vk::GraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
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
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	pipeline = deviceContext.device.createGraphicsPipelineUnique(nullptr, { pipelineInfo }, deviceContext.allocationCallbacks);
}

void vkh::GraphicsPipeline::destroy()
{
	descriptorSetLayouts.clear();
	pipelineLayout.reset();
	pipeline.reset();
}
