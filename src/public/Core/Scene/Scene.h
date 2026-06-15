#pragma once
#include <iostream>
#include <map>
#include <spdlog/spdlog.h>

#include "Core/Containers.h"

#include "Core/GameObject/GameObject.h"
#include "Core/GameObject/Components/Mesh.h"
#include "Core/Camera/Camera.h"
#include "Core/Camera/EditorCamera.h"
#include "Utils.h"

struct SceneAsset;

struct Hierarchy {
	struct HierarchyNode {
		uint32_t id = -1;

		Name name;
		WeakRef<HierarchyNode> parent;
		Vector<Ref<HierarchyNode>> children;
		GameObject* pObj = nullptr;

		/**
		* Check if hierarchy node is valid
		* 
		* @returns True if is valid
		*/
		bool
		IsValid() const {
			return this->id != -1;
		}

		/**
		* Check if hierarchy node
		* has any children
		* 
		* @returns True if it has childrens
		*/
		bool HasChildren() const { return !this->children.empty(); }
	};

	Ref<HierarchyNode> root;
	Map<uint32_t, Ref<HierarchyNode>> nodeIndices; /* ID -> Node */
	Map<Name, Vector<uint32_t>> nameIndices; /* Name -> Node IDs */

	uint32_t nCurrentId = 0;

	/**
	* Generates a unique ID
	* inside of the tree
	* 
	* @returns Generated ID
	*/
	uint32_t 
	GenerateID() {
		return this->nCurrentId++;
	}

	static Hierarchy
	Create(const Name& rootName) {
		Hierarchy hierarchy = { };
		hierarchy.root = hierarchy.CreateNode(rootName);
		return hierarchy;
	}

	/**
	* Creates a node
	* inside the tree
	* 
	* @param name Node name
	* @param parent Node parent
	* @param pObj Node GameObject
	* 
	* @returns Created node
	*/
	Ref<Hierarchy::HierarchyNode>
	CreateNode(const Name& name, Ref<HierarchyNode> parent = nullptr, GameObject* pObj = nullptr) {
		Ref<HierarchyNode> node = CreateRef<HierarchyNode>();
		node->name = name;
		node->id = this->GenerateID();
		node->parent = parent.Get();
		node->pObj = pObj;

		Vector<uint32_t>& indices = this->nameIndices[name];
		indices.push_back(node->id);

		if (parent) {
			parent->children.push_back(node);
		}

		return node;
	}

	/**
	* Changes node parent (move node)
	* 
	* @param node Hierarchy ndoe
	* @param newParent New parent
	*/
	void 
	MoveNode(Ref<HierarchyNode> node, Ref<HierarchyNode> newParent) {
		if (!node || !newParent) return;

		Ref<HierarchyNode> oldParent = node->parent.lock();
		if (oldParent) {
			Vector<Ref<HierarchyNode>>& children = oldParent->children;

			auto it = std::find_if(
				children.begin(),
				children.end(),
				[&](const Ref<HierarchyNode>& child)
				{
					return child.Get() == node.Get();
				});

			if (it != children.end())
				children.erase(it);
		}

		node->parent = newParent.Get();
		newParent->children.push_back(node);

		node->parent = newParent.Get();
		return;
	}

	/**
	* Delete a node from the hierarchy
	* 
	* @param node Hierarchy node
	*/
	void 
	DeleteNode(Ref<HierarchyNode> node) {
		if (!node) return;
		
		/* Get node parent and children */
		WeakRef<HierarchyNode> weakParent = node->parent;
		Vector<Ref<HierarchyNode>>& children = node->children;

		/* Delete each children node */
		for (Ref<HierarchyNode>& child : children) {
			this->DeleteNode(child);
		}

		children.clear();

		Ref<HierarchyNode> parent = weakParent.lock();

		if (!parent) return;

		/* Remove node from parent */
		Vector<Ref<HierarchyNode>>& parentChildren = parent->children;
		parentChildren.erase(
			std::remove_if(
				parentChildren.begin(),
				parentChildren.end(),
				[node](const Ref<HierarchyNode>& parentChild) {
					return parentChild.Get() == node.Get();
				}
			),
			parentChildren.end()
		);
	}
};

class Scene {
	friend class SceneManager;
public:
	Scene(const String& name);

	void AddObject(GameObject* object);
	Map<String, GameObject*> GetObjects();

	void Start();
	void Update();

	Camera* GetCurrentCamera() { return this->m_currentCamera; }
	Hierarchy& GetHierarchy() { return this->m_hierarchy; }

	const String 
	GetName() { 
		return this->m_name;
	}

	const SceneAsset SerializeScene();

private:
	String m_name;

	Map<String, GameObject*> m_gameObjects;

	Camera* m_currentCamera;
	Map<String, Camera*> m_cameras;

	Hierarchy m_hierarchy;
};