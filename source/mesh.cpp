#include "mesh.hpp"

#include <tiny/tiny_obj_loader.h>

#include "utility.hpp"
#include "vkhDeviceContext.hpp"

LoadedMesh loadObj(std::filesystem::path const& objPath)
{
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = "./"; // Path to material files
	tinyobj::ObjReader reader;
	
	if (!reader.ParseFromFile(objPath.string(), reader_config)) 
	{
		if (!reader.Error().empty()) {
			throw std::runtime_error(reader.Error());
		}
	}

	auto const& attrib = reader.GetAttrib();
	auto const& shapes = reader.GetShapes();
	auto const& materials = reader.GetMaterials();
	
	std::unordered_map<LoadedMesh::Vertex, uint32> uniqueVertices = {};
	LoadedMesh loadedMesh;
	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++)
	{
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
		{
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++)
			{
				LoadedMesh::Vertex vertex{};
				
				// access to vertex
				tinyobj::index_t const idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t const vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				tinyobj::real_t const vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				tinyobj::real_t const vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

				vertex.pos.x = vx;
				vertex.pos.y = vy;
				vertex.pos.z = vz;
				
				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0) {
					tinyobj::real_t const nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					tinyobj::real_t const ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					tinyobj::real_t const nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

					vertex.normal.x = nx;
					vertex.normal.y = ny;
					vertex.normal.z = nz;
				}

				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t const tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t const ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

					vertex.uv.x = tx;
					vertex.uv.y = ty;
				}

				// Optional: vertex colors
				tinyobj::real_t const red   = attrib.colors[3*size_t(idx.vertex_index)+0];
				tinyobj::real_t const green = attrib.colors[3*size_t(idx.vertex_index)+1];
				tinyobj::real_t const blue  = attrib.colors[3*size_t(idx.vertex_index)+2];

				vertex.color.r = red;
				vertex.color.g = green;
				vertex.color.b = blue;
				
				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32>(loadedMesh.vertices.size());
					loadedMesh.vertices.push_back(vertex);
				}
				loadedMesh.indices.push_back(uniqueVertices[vertex]);
			}
			
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}
	return loadedMesh;
}

Mesh::Mesh(vkh::DeviceContext& ctx, LoadedMesh const& mesh)
{
	{
		vk::BufferCreateInfo vertexBufferInfo;
		vertexBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
		vertexBufferInfo.size = mesh.vertices.size() * sizeof(mesh.vertices[0]);
		vertexBufferInfo.sharingMode = vk::SharingMode::eExclusive;

		vma::AllocationCreateInfo allocInfo;
		allocInfo.usage = vma::MemoryUsage::eCpuToGpu;
		vertexBuffer.create(ctx.gpuAllocator, vertexBufferInfo, allocInfo);
		vertexBuffer.writeData(toSpan<uint8>(mesh.vertices));
	}
	{
		vk::BufferCreateInfo indexBufferInfo;
		indexBufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
		indexBufferInfo.size = mesh.indices.size() * sizeof(mesh.indices[0]);
		indexBufferInfo.sharingMode = vk::SharingMode::eExclusive;

		vma::AllocationCreateInfo allocInfo;
		allocInfo.usage = vma::MemoryUsage::eCpuToGpu;
		indexBuffer.create(ctx.gpuAllocator, indexBufferInfo, allocInfo);
		indexBuffer.writeData(toSpan<uint8>(mesh.indices));
		indicesCount = mesh.indices.size();
	}
	{
		vk::BufferCreateInfo uniformBufferInfo;
		uniformBufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		uniformBufferInfo.size = sizeof(glm::mat4);
		uniformBufferInfo.sharingMode = vk::SharingMode::eExclusive;

		vma::AllocationCreateInfo allocInfo;
		allocInfo.usage = vma::MemoryUsage::eCpuToGpu;
		modelBuffer.create(ctx.gpuAllocator, uniformBufferInfo, allocInfo);
		
		// @TODO
		glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		
		modelBuffer.writeStruct(model);
	}
	
}

void Mesh::draw(vk::CommandBuffer cmdBuff, uint32 index)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer vertexBuffers[] = { vertexBuffer.buffer };
	
	cmdBuff.bindVertexBuffers(0, 1, vertexBuffers, offsets);
	cmdBuff.bindIndexBuffer(indexBuffer.buffer, 0, vk::IndexType::eUint32);
	cmdBuff.drawIndexed(indicesCount, 1, 0, 0, 0);
}
