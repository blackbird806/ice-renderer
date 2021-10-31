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

struct Transform
{
	glm::vec3 pos{};
	glm::vec3 scale{};
	glm::quat rot{};

	// fill pos scale and rot from mat
	void decompose();

	// update mat through (pos * rot * scale) operation
	void updateMat();
	
	glm::mat4 mat = glm::mat4(1.0f);
};

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
	// helper struct that represent an object/Node in the scene
	struct Object
	{
		Object& setRenderObject(RenderObject const& renderObj);
		Object& setName(std::string const& name);
		Object& setLocalMatrix(glm::mat4 const& mtr);
		Object& setTransform(Transform const& tr);
		Object& setTransformNoDirty(Transform const& tr);

		std::string getName() const;
		int32 getLevel() const;
		Transform const& getTransform() const;
		// return null if no renderobject exist for that node
		RenderObject* getRenderObject() const;
		
		HierarchyID nodeId;
		Scene& scene;
	};

	struct Iterator
	{
		Iterator& operator++();

		bool operator!=(Iterator const&) const;
		Object operator*() const;
		
		HierarchyID id;
		Scene& scene;
	};
	
	HierarchyID addNode(HierarchyID parent);
	Object addObject(HierarchyID parent);
	Object getObject(HierarchyID id);
	
	void markDirty(HierarchyID node);
	void computeWorldsTransforms();

	void imguiDrawSceneTree();
	void imguiDrawSceneTreeLevel(Iterator first);
	void imguiDrawInspector();

	Iterator begin();
	Iterator end();
	
	std::vector<Hierarchy> hierarchy;
	std::vector<std::string> nodeNames;
	std::vector<Transform> localTransforms;
	std::vector<glm::mat4> worlds;

	std::vector<std::vector<HierarchyID>> dirtyNodes;
	int32 maxLevel = 0;
	
	HierarchyID focusedId = invalidNodeID;

	std::unordered_map<HierarchyID, RenderObject> renderObjects;
	
	// render resources
	std::vector<Mesh> meshes;
	std::vector<Material> materials;
};