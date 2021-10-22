#include "scenegraph.hpp"
#undef min
#undef max

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui/imgui.h>
#include <format>
#include <algorithm>


void Transform::decompose()
{
	glm::vec3 skew; // unused
	glm::vec4 perspective; // unused
	glm::decompose(mat, scale, rot, pos, skew, perspective);
}

void Transform::updateMat()
{
	mat = glm::translate(glm::mat4(1.0f), pos);
	mat *= glm::toMat4(rot);
	mat = glm::scale(mat, scale);
}

Scene::Object& Scene::Object::setRenderObject(RenderObject const& renderObj)
{
	scene.renderObjects[nodeId] = renderObj;
	return *this;
}

Scene::Object& Scene::Object::setName(std::string const& name)
{
	scene.nodeNames[nodeId] = name;
	return *this;
}

Scene::Object& Scene::Object::setLocalMatrix(glm::mat4 const& mtr)
{
	scene.localTransforms[nodeId].mat = mtr;
	scene.localTransforms[nodeId].decompose();
	
	scene.markDirty(nodeId);
	return *this;
}

Scene::Object& Scene::Object::setTransform(Transform const& tr)
{
	scene.markDirty(nodeId);
	return setTransformNoDirty(tr);
}

Scene::Object& Scene::Object::setTransformNoDirty(Transform const& tr)
{
	scene.localTransforms[nodeId] = tr;
	return *this;
}

std::string Scene::Object::getName() const
{
	return scene.nodeNames[nodeId];
}

int32 Scene::Object::getLevel() const
{
	return scene.hierarchy[nodeId].level;
}

Transform const& Scene::Object::getTransform() const
{
	return scene.localTransforms[nodeId];
}

RenderObject* Scene::Object::getRenderObject() const
{
	auto const it = scene.renderObjects.find(nodeId);
	if (it != scene.renderObjects.end())
		return &it->second;
	return nullptr;
}

Scene::Iterator& Scene::Iterator::operator++()
{
	Hierarchy const h = scene.hierarchy[id];
	if (h.nextSibling != invalidNodeID)
	{
		id = h.nextSibling;
		return *this;
	}
	id = h.firstChild;
	return *this;
}

bool Scene::Iterator::operator!=(Iterator const& rhs) const
{
	return id != rhs.id;
}

Scene::Object Scene::Iterator::operator*() const
{
	return { id, scene };
}

HierarchyID Scene::addNode(HierarchyID parent)
{
	HierarchyID const newNode = hierarchy.size();
	localTransforms.push_back({});
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
	maxLevel = std::max(maxLevel, hierarchy[newNode].level);
	return newNode;
}

Scene::Object Scene::addObject(HierarchyID parent)
{
	return Object{ addNode(parent), *this };
}

void Scene::markDirty(HierarchyID node)
{
	dirtyNodes.resize(maxLevel + 1);
	int32 const level = hierarchy[node].level;
	dirtyNodes[level].push_back(node);

	for (HierarchyID current = hierarchy[node].firstChild; current != invalidNodeID; current = hierarchy[current].nextSibling)
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
			localTransforms[node].updateMat();
			// TODO handle this better
			if (parent == invalidNodeID)
			{
				worlds[node] = localTransforms[node].mat;
				continue;
			}
			worlds[node] = worlds[parent] * localTransforms[node].mat;
		}
	}
	dirtyNodes.clear();
}

void Scene::imguiDrawSceneTree()
{
	if (ImGui::TreeNode("Scene"))
	{
		imguiDrawSceneTreeLevel(begin());
		ImGui::TreePop();
	}
}

void Scene::imguiDrawSceneTreeLevel(Iterator it)
{
	int32 const level = (*it).getLevel();
	
	if (ImGui::TreeNode((*it).getName().c_str()))
	{
		for (int i = 0; it != end(); ++it)
		{
			auto node = *it;
			if (level != node.getLevel())
			{
				imguiDrawSceneTreeLevel(it);
				break;
			}

			if (ImGui::Selectable(node.getName().c_str()))
			{
				
			}
			
			Transform transform = node.getTransform();
			
			if (ImGui::DragFloat3("pos", (float*)&transform.pos))
				node.setTransform(transform);
			if (ImGui::DragFloat3("scale", (float*)&transform.scale))
				node.setTransform(transform);

		}
		ImGui::TreePop();
	}
}

Scene::Iterator Scene::begin()
{
	// @Review
	// assume root is 0
	return {0, *this};
}

Scene::Iterator Scene::end()
{
	return { invalidNodeID, *this };
}