#pragma once

#include <filesystem>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vkhBuffer.hpp>
#include <vkhCommandBuffers.hpp>

#include "Material.hpp"

namespace vkh {
	struct DeviceContext;
}

struct LoadedMesh
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
};

struct ModelBuffer
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

// @TODO true hash function
namespace std {
	template<> struct hash<LoadedMesh::Vertex> {
		size_t operator()(LoadedMesh::Vertex const& vertex) const {
			return ((((
					(hash<glm::vec3>()(vertex.pos) ^
					(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
					(hash<glm::vec3>()(vertex.normal) << 2)) >> 2)) ^
					(hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}

LoadedMesh loadObj(std::filesystem::path const& objPath);

class Mesh
{
public:
	Mesh(vkh::DeviceContext& ctx, LoadedMesh const& mesh);

	void draw(vk::CommandBuffer cmdBuff, uint32);

	Material material;
	vkh::Buffer vertexBuffer;
	vkh::Buffer indexBuffer;
	vkh::Buffer uniformBuffer;
};