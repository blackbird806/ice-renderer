#include "scenegraph.hpp"

#include <imgui/imgui.h>
#include <format>
#include <algorithm>

#undef min
#undef max

HierarchyID Scene::addNode(HierarchyID parent)
{
	assert(parent < hierarchy.size());
	
	HierarchyID const newNode = hierarchy.size();
	locals.push_back(glm::mat4(1.0f));
	worlds.push_back(glm::mat4(1.0f));
	nodeNames.push_back(std::format("default_{}", newNode));
	hierarchy.push_back({ .parent = parent });

	if (parent != invalidNodeID)
	{
		HierarchyID const s = hierarchy[parent].firstChild;
		if (s == invalidNodeID)
		{
			hierarchy[parent].firstChild = newNode;
			hierarchy[newNode].lastSibling = newNode;
		}
		else
		{
			HierarchyID dest = hierarchy[s].lastSibling;
			if (dest == invalidNodeID)
			{
				for (dest = s; dest != invalidNodeID; dest = hierarchy[dest].nextSibling);
			}
			hierarchy[dest].nextSibling = newNode;
			hierarchy[s].lastSibling = newNode;
		}
		hierarchy[newNode].level = hierarchy[parent].level + 1;
	}
	else
	{
		// newNode is root
		hierarchy[newNode].level = 0;
	}
	maxLevel = std::max(maxLevel, hierarchy[newNode].level);
	return newNode;
}

void Scene::markDirty(HierarchyID node)
{
	dirtyNodes.resize(maxLevel);
	int32 const level = hierarchy[node].level;
	dirtyNodes[level].push_back(node);

	for (HierarchyID current = node; current != invalidNodeID; current = hierarchy[current].nextSibling)
	{
		markDirty(current);
	}
}

void Scene::computeWorldsTransforms()
{
	for (auto& level : dirtyNodes)
	{
		for (auto node : level)
		{
			HierarchyID const parent = hierarchy[node].parent;
			worlds[node] = worlds[parent] * locals[node];
		}
		level.clear();
	}
	dirtyNodes.clear();
}

void Scene::imguiDrawSceneTree()
{
	// TODO
}
