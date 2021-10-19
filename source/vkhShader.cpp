#include "vkhShader.hpp"

#include <SPIRV-Reflect/spirv_reflect.h>


#include "utility.hpp"
#include "vkhDeviceContext.hpp"

using namespace vkh;

// @Improve use SPVRflect C++ API

ShaderReflector::ShaderReflector(ShaderReflector&& rhs) noexcept : module(rhs.module)
{
	rhs.isValid = false;
	isValid = true;
}

ShaderReflector& ShaderReflector::operator=(ShaderReflector&& rhs) noexcept
{
	module = rhs.module;
	rhs.isValid = false;
	isValid = true;
	return *this;
}

void ShaderReflector::create(std::span<uint8 const> spvCode)
{
	SpvReflectResult result = spvReflectCreateShaderModule(spvCode.size_bytes(), spvCode.data(), &module);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);
	isValid = true;
}

void ShaderReflector::destroy()
{
	if (isValid)
	{
		spvReflectDestroyShaderModule(&module);
		isValid = false;
	}
}

ShaderReflector::~ShaderReflector()
{
	destroy();
}

size_t ShaderReflector::DescriptorSetDescriptor::Struct::getSize() const
{
	size_t sum = 0;
	for (auto const& member : members)
		sum += member.getSize();
	return sum;
}

size_t ShaderReflector::DescriptorSetDescriptor::Member::getSize() const
{
	return std::visit(overloaded{
	[](auto&& e)
	{
		return sizeof(e);
	},
	[] (Struct const& s)
	{
		return s.getSize();
	}
	}, value);
	return 0;
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

//@TODO precompute shader reflexion

bool ShaderReflector::DescriptorSetLayoutData::operator==(DescriptorSetLayoutData const& rhs) const noexcept
{
	return set_number == rhs.set_number;
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
	
	for (auto* const var : inputVars)
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
	auto sets = reflectDescriptorSets();

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
	}
	return set_layouts;
}

std::vector<SpvReflectDescriptorSet*> ShaderReflector::reflectDescriptorSets() const
{
	uint32_t count = 0;
	SpvReflectResult result = spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	std::vector<SpvReflectDescriptorSet*> sets(count);
	result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	return sets;
}

// @Improve only float are supported for now
static ShaderReflector::DescriptorSetDescriptor::Member reflectMember(SpvReflectTypeDescription const& typeDescription)
{
	ShaderReflector::DescriptorSetDescriptor::Member mem;
	if (typeDescription.struct_member_name != nullptr)
		mem.name = typeDescription.struct_member_name;
	mem.typeFlags = typeDescription.type_flags;

	auto const& traits = typeDescription.traits;
	if (mem.typeFlags & SPV_REFLECT_TYPE_FLAG_ARRAY)
	{
		mem.arrayTraits = traits.array;
	}
	if (mem.typeFlags & SPV_REFLECT_TYPE_FLAG_MATRIX)
	{
#define ROW_CASE_CASE(COLUMN_DIM, ROW_DIM) case ROW_DIM: mem.value = glm::mat##COLUMN_DIM##x##ROW_DIM##(); break;
#define ROW_CASE(COLUMN_DIM) case COLUMN_DIM: switch(traits.numeric.matrix.row_count) { \
		ROW_CASE_CASE(COLUMN_DIM, 2) \
		ROW_CASE_CASE(COLUMN_DIM, 3) \
		ROW_CASE_CASE(COLUMN_DIM, 4) \
			default:\
		throw std::runtime_error("unsuported vector dimension");\
		} break;
		
		switch(traits.numeric.matrix.column_count)
		{
			ROW_CASE(2)
			ROW_CASE(3)
			ROW_CASE(4)
		default:
				throw std::runtime_error("unsuported vector dimension");
		}
#undef ROW_CASE_CASE
#undef ROW_CASE
	}
	else if (mem.typeFlags & SPV_REFLECT_TYPE_FLAG_VECTOR)
	{
#define COMPONENT_CASE(DIM) case DIM: mem.value = glm::vec##DIM##();break;
		switch (traits.numeric.vector.component_count)
		{
			COMPONENT_CASE(1)
			COMPONENT_CASE(2)
			COMPONENT_CASE(3)
			COMPONENT_CASE(4)
		default:
			throw std::runtime_error("unsuported vector dimension");
		}
#undef COMPONENT_CASE;
	}
	else if (mem.typeFlags & SPV_REFLECT_TYPE_FLAG_FLOAT)
	{
		if (traits.numeric.scalar.width == 32)
			mem.value = float(0);
		else if (traits.numeric.scalar.width == 64)
			mem.value = double(0);
		else
			throw std::runtime_error("unsuported float width");
	}
	else if (mem.typeFlags & SPV_REFLECT_TYPE_FLAG_BOOL)
		mem.value = false;
	else if (mem.typeFlags & SPV_REFLECT_TYPE_FLAG_INT)
	{
#define CASE_WIDTH(W) case W: mem.value =  traits.numeric.scalar.signedness > 0 ? int##W##(0) : uint##W##(0); break;
		switch (traits.numeric.scalar.width)
		{
			CASE_WIDTH(8)
			CASE_WIDTH(16)
			CASE_WIDTH(32)
			CASE_WIDTH(64)
		default:
			throw std::runtime_error("unsuported int width");
		}
#undef CASE_WIDTH
	}
	else if (mem.typeFlags & SPV_REFLECT_TYPE_FLAG_STRUCT)
	{
		// @Review ugly code right here
		ShaderReflector::DescriptorSetDescriptor::Struct struct_;
		for (int j_member = 0; j_member < typeDescription.member_count; j_member++)
		{
			struct_.members.emplace_back(reflectMember(typeDescription.members[j_member]));
		}
		mem.value = std::move(struct_);
	}
	else
		throw std::runtime_error("unsuported type");
	
	return mem;
}

static ShaderReflector::DescriptorSetDescriptor::Binding reflectDescriptorBinding(SpvReflectDescriptorBinding const& reflBinding)
{
	ShaderReflector::DescriptorSetDescriptor::Binding binding;
	binding.descriptorType = reflBinding.descriptor_type;
	binding.element = reflectMember(*reflBinding.type_description);
	if (!std::holds_alternative<ShaderReflector::DescriptorSetDescriptor::Struct>(binding.element.value))
		binding.element.name = reflBinding.name;
	return binding;
}

std::vector<ShaderReflector::DescriptorSetDescriptor> ShaderReflector::createDescriptorSetDescriptors() const
{
	std::vector<DescriptorSetDescriptor> descriptors;
	auto const sets = reflectDescriptorSets();
	for (uint setNum = 0; auto const& set : sets)
	{
		ShaderReflector::DescriptorSetDescriptor desc;
		for (uint32_t i_binding = 0; i_binding < set->binding_count; ++i_binding)
		{
			const SpvReflectDescriptorBinding& refl_binding = *(set->bindings[i_binding]);
			desc.bindings.push_back(reflectDescriptorBinding(refl_binding));
		}
		desc.setNumber = set->set;
		descriptors.push_back(std::move(desc));
		setNum++;
	}
	
	return descriptors;
}

vk::ShaderStageFlagBits ShaderReflector::getShaderStage() const
{
	return static_cast<vk::ShaderStageFlagBits>(module.shader_stage);
}

void ShaderModule::create(vkh::DeviceContext& ctx, vk::ShaderStageFlagBits shaderStage_, std::span<uint8> data)
{
	vk::ShaderModuleCreateInfo fragmentShaderCreateInfo{};
	fragmentShaderCreateInfo.codeSize = data.size();
	fragmentShaderCreateInfo.pCode = reinterpret_cast<uint32_t const*>(data.data());
	shaderStage = shaderStage_;
	
	module = ctx.device.createShaderModuleUnique(fragmentShaderCreateInfo, ctx.allocationCallbacks);
	reflector.create(data);
}

void ShaderModule::destroy()
{
	module.reset();
	reflector.destroy();
}

vk::PipelineShaderStageCreateInfo ShaderModule::getPipelineShaderStage() const
{
	vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{};
	pipelineShaderStageCreateInfo.stage = shaderStage;
	pipelineShaderStageCreateInfo.module = *module;
	pipelineShaderStageCreateInfo.pName = "main";
	
	return pipelineShaderStageCreateInfo;
}

