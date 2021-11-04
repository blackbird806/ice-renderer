#pragma once

#include <filesystem>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vkhBuffer.hpp>
#include <vkhCommandBuffers.hpp>
#include <tiny/tiny_obj_loader.h>

#include "renderObject.hpp"

namespace vkh {
	struct DeviceContext;
}

struct LoadedObj
{
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 color;
		glm::vec2 uv;

		bool operator==(Vertex const& other) const
		{
			return pos == other.pos && uv == other.uv && color == other.color && normal == other.normal;
		}
	};
	
	std::vector<Vertex> vertices;
	std::vector<uint32> indices;
	std::vector<tinyobj::material_t> materials;
};

// @Review better hash function ?
namespace std {
	template<> struct hash<LoadedObj::Vertex> {
		size_t operator()(LoadedObj::Vertex const& vertex) const {
			return ((((
					(hash<glm::vec3>()(vertex.pos) ^
					(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
					(hash<glm::vec3>()(vertex.normal) << 2)) >> 2)) ^
					(hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}

LoadedObj loadObj(std::filesystem::path const& objPath);

class Mesh
{
public:
	Mesh(vkh::DeviceContext& ctx, LoadedObj const& mesh, uint32 maxFramesInFlight);

	void draw(vk::CommandBuffer cmdBuff);

	size_t indicesCount;
	
	vkh::Buffer vertexBuffer;
	vkh::Buffer indexBuffer;

	vkh::Buffer modelBuffer;
	std::vector<vk::DescriptorSet> modelSets;
};