#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

#include "ice.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "renderObject.hpp"

using HierarchyID = int32;

HierarchyID constexpr invalidNodeID = -1;

struct Hierarchy
{
	HierarchyID parent = invalidNodeID;
	HierarchyID firstChild = invalidNodeID;
	HierarchyID nextSibling = invalidNodeID;
	HierarchyID lastSibling = invalidNodeID;
	int32 level;
};

struct Scene
{
	HierarchyID addNode(HierarchyID parent);
	void markDirty(HierarchyID node);
	void computeWorldsTransforms();
	
	void imguiDrawSceneTree();

	std::vector<Hierarchy> hierarchy;
	std::vector<std::string> nodeNames;
	std::vector<glm::mat4> locals;
	std::vector<glm::mat4> worlds;

	std::vector<std::vector<HierarchyID>> dirtyNodes;
	int32 maxLevel = 0;

	std::unordered_map<HierarchyID, RenderObject> renderObjects;
	
	// render resources
	std::vector<Mesh> meshes;
	std::vector<Material> materials;
};