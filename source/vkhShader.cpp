#include "vkhShader.hpp"

#include <iostream>
#include <SPIRV-Reflect/spirv_reflect.h>

#include "vkhDeviceContext.hpp"

using namespace vkh;

ShaderReflector::ShaderReflector(std::span<uint8 const> spvCode)
{
	SpvReflectResult result = spvReflectCreateShaderModule(spvCode.size_bytes(), spvCode.data(), &module);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);
}

ShaderReflector::~ShaderReflector()
{
	spvReflectDestroyShaderModule(&module);
}

// Returns the size in bytes of the provided VkFormat.
// As this is only intended for vertex attribute formats, not all VkFormats are supported.
static uint32_t formatSize(VkFormat format)
{
	uint32_t result = 0;
	switch (format) {
	case VK_FORMAT_UNDEFINED: result = 0; break;
	case VK_FORMAT_R4G4_UNORM_PACK8: result = 1; break;
	case VK_FORMAT_R4G4B4A4_UNORM_PACK16: result = 2; break;
	case VK_FORMAT_B4G4R4A4_UNORM_PACK16: result = 2; break;
	case VK_FORMAT_R5G6B5_UNORM_PACK16: result = 2; break;
	case VK_FORMAT_B5G6R5_UNORM_PACK16: result = 2; break;
	case VK_FORMAT_R5G5B5A1_UNORM_PACK16: result = 2; break;
	case VK_FORMAT_B5G5R5A1_UNORM_PACK16: result = 2; break;
	case VK_FORMAT_A1R5G5B5_UNORM_PACK16: result = 2; break;
	case VK_FORMAT_R8_UNORM: result = 1; break;
	case VK_FORMAT_R8_SNORM: result = 1; break;
	case VK_FORMAT_R8_USCALED: result = 1; break;
	case VK_FORMAT_R8_SSCALED: result = 1; break;
	case VK_FORMAT_R8_UINT: result = 1; break;
	case VK_FORMAT_R8_SINT: result = 1; break;
	case VK_FORMAT_R8_SRGB: result = 1; break;
	case VK_FORMAT_R8G8_UNORM: result = 2; break;
	case VK_FORMAT_R8G8_SNORM: result = 2; break;
	case VK_FORMAT_R8G8_USCALED: result = 2; break;
	case VK_FORMAT_R8G8_SSCALED: result = 2; break;
	case VK_FORMAT_R8G8_UINT: result = 2; break;
	case VK_FORMAT_R8G8_SINT: result = 2; break;
	case VK_FORMAT_R8G8_SRGB: result = 2; break;
	case VK_FORMAT_R8G8B8_UNORM: result = 3; break;
	case VK_FORMAT_R8G8B8_SNORM: result = 3; break;
	case VK_FORMAT_R8G8B8_USCALED: result = 3; break;
	case VK_FORMAT_R8G8B8_SSCALED: result = 3; break;
	case VK_FORMAT_R8G8B8_UINT: result = 3; break;
	case VK_FORMAT_R8G8B8_SINT: result = 3; break;
	case VK_FORMAT_R8G8B8_SRGB: result = 3; break;
	case VK_FORMAT_B8G8R8_UNORM: result = 3; break;
	case VK_FORMAT_B8G8R8_SNORM: result = 3; break;
	case VK_FORMAT_B8G8R8_USCALED: result = 3; break;
	case VK_FORMAT_B8G8R8_SSCALED: result = 3; break;
	case VK_FORMAT_B8G8R8_UINT: result = 3; break;
	case VK_FORMAT_B8G8R8_SINT: result = 3; break;
	case VK_FORMAT_B8G8R8_SRGB: result = 3; break;
	case VK_FORMAT_R8G8B8A8_UNORM: result = 4; break;
	case VK_FORMAT_R8G8B8A8_SNORM: result = 4; break;
	case VK_FORMAT_R8G8B8A8_USCALED: result = 4; break;
	case VK_FORMAT_R8G8B8A8_SSCALED: result = 4; break;
	case VK_FORMAT_R8G8B8A8_UINT: result = 4; break;
	case VK_FORMAT_R8G8B8A8_SINT: result = 4; break;
	case VK_FORMAT_R8G8B8A8_SRGB: result = 4; break;
	case VK_FORMAT_B8G8R8A8_UNORM: result = 4; break;
	case VK_FORMAT_B8G8R8A8_SNORM: result = 4; break;
	case VK_FORMAT_B8G8R8A8_USCALED: result = 4; break;
	case VK_FORMAT_B8G8R8A8_SSCALED: result = 4; break;
	case VK_FORMAT_B8G8R8A8_UINT: result = 4; break;
	case VK_FORMAT_B8G8R8A8_SINT: result = 4; break;
	case VK_FORMAT_B8G8R8A8_SRGB: result = 4; break;
	case VK_FORMAT_A8B8G8R8_UNORM_PACK32: result = 4; break;
	case VK_FORMAT_A8B8G8R8_SNORM_PACK32: result = 4; break;
	case VK_FORMAT_A8B8G8R8_USCALED_PACK32: result = 4; break;
	case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: result = 4; break;
	case VK_FORMAT_A8B8G8R8_UINT_PACK32: result = 4; break;
	case VK_FORMAT_A8B8G8R8_SINT_PACK32: result = 4; break;
	case VK_FORMAT_A8B8G8R8_SRGB_PACK32: result = 4; break;
	case VK_FORMAT_A2R10G10B10_UNORM_PACK32: result = 4; break;
	case VK_FORMAT_A2R10G10B10_SNORM_PACK32: result = 4; break;
	case VK_FORMAT_A2R10G10B10_USCALED_PACK32: result = 4; break;
	case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: result = 4; break;
	case VK_FORMAT_A2R10G10B10_UINT_PACK32: result = 4; break;
	case VK_FORMAT_A2R10G10B10_SINT_PACK32: result = 4; break;
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32: result = 4; break;
	case VK_FORMAT_A2B10G10R10_SNORM_PACK32: result = 4; break;
	case VK_FORMAT_A2B10G10R10_USCALED_PACK32: result = 4; break;
	case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: result = 4; break;
	case VK_FORMAT_A2B10G10R10_UINT_PACK32: result = 4; break;
	case VK_FORMAT_A2B10G10R10_SINT_PACK32: result = 4; break;
	case VK_FORMAT_R16_UNORM: result = 2; break;
	case VK_FORMAT_R16_SNORM: result = 2; break;
	case VK_FORMAT_R16_USCALED: result = 2; break;
	case VK_FORMAT_R16_SSCALED: result = 2; break;
	case VK_FORMAT_R16_UINT: result = 2; break;
	case VK_FORMAT_R16_SINT: result = 2; break;
	case VK_FORMAT_R16_SFLOAT: result = 2; break;
	case VK_FORMAT_R16G16_UNORM: result = 4; break;
	case VK_FORMAT_R16G16_SNORM: result = 4; break;
	case VK_FORMAT_R16G16_USCALED: result = 4; break;
	case VK_FORMAT_R16G16_SSCALED: result = 4; break;
	case VK_FORMAT_R16G16_UINT: result = 4; break;
	case VK_FORMAT_R16G16_SINT: result = 4; break;
	case VK_FORMAT_R16G16_SFLOAT: result = 4; break;
	case VK_FORMAT_R16G16B16_UNORM: result = 6; break;
	case VK_FORMAT_R16G16B16_SNORM: result = 6; break;
	case VK_FORMAT_R16G16B16_USCALED: result = 6; break;
	case VK_FORMAT_R16G16B16_SSCALED: result = 6; break;
	case VK_FORMAT_R16G16B16_UINT: result = 6; break;
	case VK_FORMAT_R16G16B16_SINT: result = 6; break;
	case VK_FORMAT_R16G16B16_SFLOAT: result = 6; break;
	case VK_FORMAT_R16G16B16A16_UNORM: result = 8; break;
	case VK_FORMAT_R16G16B16A16_SNORM: result = 8; break;
	case VK_FORMAT_R16G16B16A16_USCALED: result = 8; break;
	case VK_FORMAT_R16G16B16A16_SSCALED: result = 8; break;
	case VK_FORMAT_R16G16B16A16_UINT: result = 8; break;
	case VK_FORMAT_R16G16B16A16_SINT: result = 8; break;
	case VK_FORMAT_R16G16B16A16_SFLOAT: result = 8; break;
	case VK_FORMAT_R32_UINT: result = 4; break;
	case VK_FORMAT_R32_SINT: result = 4; break;
	case VK_FORMAT_R32_SFLOAT: result = 4; break;
	case VK_FORMAT_R32G32_UINT: result = 8; break;
	case VK_FORMAT_R32G32_SINT: result = 8; break;
	case VK_FORMAT_R32G32_SFLOAT: result = 8; break;
	case VK_FORMAT_R32G32B32_UINT: result = 12; break;
	case VK_FORMAT_R32G32B32_SINT: result = 12; break;
	case VK_FORMAT_R32G32B32_SFLOAT: result = 12; break;
	case VK_FORMAT_R32G32B32A32_UINT: result = 16; break;
	case VK_FORMAT_R32G32B32A32_SINT: result = 16; break;
	case VK_FORMAT_R32G32B32A32_SFLOAT: result = 16; break;
	case VK_FORMAT_R64_UINT: result = 8; break;
	case VK_FORMAT_R64_SINT: result = 8; break;
	case VK_FORMAT_R64_SFLOAT: result = 8; break;
	case VK_FORMAT_R64G64_UINT: result = 16; break;
	case VK_FORMAT_R64G64_SINT: result = 16; break;
	case VK_FORMAT_R64G64_SFLOAT: result = 16; break;
	case VK_FORMAT_R64G64B64_UINT: result = 24; break;
	case VK_FORMAT_R64G64B64_SINT: result = 24; break;
	case VK_FORMAT_R64G64B64_SFLOAT: result = 24; break;
	case VK_FORMAT_R64G64B64A64_UINT: result = 32; break;
	case VK_FORMAT_R64G64B64A64_SINT: result = 32; break;
	case VK_FORMAT_R64G64B64A64_SFLOAT: result = 32; break;
	case VK_FORMAT_B10G11R11_UFLOAT_PACK32: result = 4; break;
	case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: result = 4; break;

	default:
		break;
	}
	return result;
}

// https://github.com/KhronosGroup/SPIRV-Reflect/blob/master/examples/main_io_variables.cpp
ShaderReflector::VertexDescription ShaderReflector::getVertexDescriptions() const
{
	// Enumerate and extract shader's input variables
	uint32_t var_count = 0;
	SpvReflectResult result = spvReflectEnumerateInputVariables(&module, &var_count, nullptr);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);
	
	std::vector<SpvReflectInterfaceVariable*> inputVars(var_count);
	result = spvReflectEnumerateInputVariables(&module, &var_count, inputVars.data());
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	// Output variables, descriptor bindings, descriptor sets, and push constants
	// can be enumerated and extracted using a similar mechanism.

	std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
	attributeDescriptions.reserve(inputVars.size());
	
	for (auto const var : inputVars)
	{
		vk::VertexInputAttributeDescription attributeDescription;
		attributeDescription.location = var->location;
		attributeDescription.format = static_cast<vk::Format>(var->format);
		attributeDescription.binding = 0;
		attributeDescriptions.push_back(attributeDescription);

	}
	// Sort attributes by location
	std::sort(std::begin(attributeDescriptions), std::end(attributeDescriptions),
		[] (const vk::VertexInputAttributeDescription& a, const vk::VertexInputAttributeDescription& b) 
		{
			return a.location < b.location;
		});
	
	vk::VertexInputBindingDescription bindingDescription;
	bindingDescription.inputRate = vk::VertexInputRate::eVertex;
	
	// Compute final offsets of each attribute, and total vertex stride.
	for (auto& attribute : attributeDescriptions) 
	{
		uint32_t const format_size = formatSize((VkFormat)attribute.format);
		attribute.offset = bindingDescription.stride;
		bindingDescription.stride += format_size;
	}
	
	return VertexDescription{attributeDescriptions, bindingDescription};
}

// https://github.com/KhronosGroup/SPIRV-Reflect/blob/master/examples/main_descriptors.cpp
// TODO get push constants
std::vector<ShaderReflector::DescriptorSetLayoutData> ShaderReflector::getDescriptorSetLayoutData() const
{
	uint32_t count = 0;
	SpvReflectResult result = spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	std::vector<SpvReflectDescriptorSet*> sets(count);
	result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	std::vector<DescriptorSetLayoutData> set_layouts(sets.size());
	
	for (size_t i_set = 0; i_set < sets.size(); ++i_set) 
	{
		const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);
		DescriptorSetLayoutData& layout = set_layouts[i_set];
		layout.bindings.resize(refl_set.binding_count);
		
		for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding) 
		{
			const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);
			vk::DescriptorSetLayoutBinding& layout_binding = layout.bindings[i_binding];
			layout_binding.binding = refl_binding.binding;
			layout_binding.descriptorType = static_cast<vk::DescriptorType>(refl_binding.descriptor_type);
			layout_binding.descriptorCount = 1;
			for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim) 
			{
				layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
			}
			layout_binding.stageFlags = static_cast<vk::ShaderStageFlagBits>(module.shader_stage);
		}
		layout.set_number = refl_set.set;
		layout.create_info.bindingCount = refl_set.binding_count;
		layout.create_info.pBindings = layout.bindings.data();
	}
	return set_layouts;
}

void Shader::create(DeviceContext* deviceContext, std::span<uint8 const> spvCode)
{
	assert(deviceContext);
	
	vk::ShaderModuleCreateInfo shaderCreateInfo{};
	shaderCreateInfo.codeSize = spvCode.size_bytes();
	shaderCreateInfo.pCode = reinterpret_cast<uint32_t const*>(spvCode.data());

	shaderModule = deviceContext->device.createShaderModuleUnique(shaderCreateInfo, deviceContext->allocationCallbacks);
}

void Shader::destroy()
{
	shaderModule.reset();
}
