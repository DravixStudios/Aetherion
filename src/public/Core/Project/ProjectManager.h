#pragma once
#include "Utils.h"
#include "Core/Containers.h"

#include "Core/Resources/AssetManager.h"
#include "Core/Resources/MeshAsset.h"
#include "Core/Resources/TextureAsset.h"
#include "Core/Resources/ProjectAsset.h"

#include <variant>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

namespace ProjectManagerHelpers {
	/**
	* Get asset displayName
	* 
	* @param asset Asset variant
	* 
	* @returns Asset display name
	*/
	inline const Name&
	GetAssetName(const AssetHandle& handle)
	{
		const AssetVariant& asset =
			AssetManager::GetInstance()->GetAsset(handle);

		return std::visit([](const auto& a) -> const Name& {
				return a.header.displayName;
			}, asset);
	}
}

struct Directory {
	String name;

	bool IsValid() const { return !this->name.empty(); }
	bool Exists() const { return !this->name.empty() && fs::is_directory(this->name) && fs::exists(this->name); }

	bool operator==(const Directory& dir) { return this->name == dir.name; }
};

struct ProjectTree {
	struct TreeNode {
		uint32_t id = -1;

		Directory dir;
		Map<uint32_t, Ref<TreeNode>> subNodes;
		Vector<AssetHandle> assets;

		WeakRef<TreeNode> parent;

		uint32_t nSubNodes() const { return static_cast<uint32_t>(subNodes.size()); }
	};

	/* Tree root node */
	Ref<TreeNode> root;

	/* Asset and node indexing */
	Map<uint32_t, Ref<TreeNode>> nodeIndices; /* ID -> Node */
	Map<Name, Vector<uint32_t>> assetIndices; /* Name -> Node IDs */
	
	/* Current unique ID */
	uint32_t currentId = 0;

	/**
	* Static method that creates
	* a new tree based on the 
	* root directory
	* 
	* @param rootDir Root directory
	* 
	* @returns Created project tree
	*/
	static ProjectTree
	Create(const Directory& rootDir) {
		ProjectTree tree = { };
		tree.root = tree.CreateNode(rootDir);
		return tree;
	}

	/**
	* Generates a unique ID
	* inside of the tree
	* 
	* @returns Generated ID
	*/
	uint32_t 
	GenerateID() {
		return this->currentId++;
	}

	/**
	* Helper function that creates
	* a node inside of the tree
	* 
	* @param dir Directory
	* @param parent Node parent (optional)
	* 
	* @returns Created node
	*/
	Ref<TreeNode> 
	CreateNode(const Directory& dir, Ref<TreeNode> parent = nullptr) {
		Ref<TreeNode> node = CreateRef<TreeNode>();
		node->id = this->GenerateID();
		node->dir = dir;
		node->parent = parent.Get();

		this->nodeIndices[node->id] = node;

		/* Add node to the parent */
		if (parent) {
			parent->subNodes[node->id] = node;
		}

		return node;
	}

	/**
	* Gets a node by ID
	* 
	* @param id Node ID
	* 
	* @returns The node if found. nullptr if not found
	*/
	Ref<TreeNode>
	GetNode(uint32_t id) {
		if (this->nodeIndices.contains(id)) {
			return this->nodeIndices[id];
		}

		return nullptr;
	}

	/**
	* Changes node parent (move node)
	* 
	* @param node Tree node
	* @param newParent New parent
	*
	* @returns True if success
	*/
	bool 
	MoveNode(Ref<TreeNode> node, Ref<TreeNode> newParent) {
		if (!node || !newParent) return false;

		/* Remove node from old parent */
		Ref<TreeNode> oldParent = node->parent.lock();
		if (oldParent) {
			oldParent->subNodes.erase(node->id);
		}

		node->parent = newParent.Get();
		newParent->subNodes[node->id] = node;
		return true;
	}

	/**
	* Recursively removes
	* a node from the tree
	* 
	* @param node Tree node
	*/
	void 
	RemoveNodeRecursive(Ref<TreeNode> node) {
		if (!node) return;

		/* Remove assets from index */
		for (AssetHandle& handle : node->assets) {
			Name name = ProjectManagerHelpers::GetAssetName(handle);
			Vector<uint32_t>& vec = this->assetIndices[name];
			vec.erase(std::remove(vec.begin(), vec.end(), node->id), vec.end());
		}

		/* Recursive over childs */
		for (auto& [id, child] : node->subNodes) {
			this->RemoveNodeRecursive(child);
		}
		this->nodeIndices.erase(node->id);
	}

	/**
	* Removes a node 
	* from the tree
	* 
	* @param node Tree node
	* 
	* @returns True if success
	*/
	bool 
	DeleteNode(Ref<TreeNode> node) {
		if(!node) return false;

		this->RemoveNodeRecursive(node);
		Ref<TreeNode> parent = node->parent.lock();
		if(parent) parent->subNodes.erase(node->id);

		node = nullptr;

		return true;
	}

	/**
	* Adds the specified asset
	* to the specified node
	* 
	* @param node Tree node
	* @param asset Added asset
	*/
	void
	AddAsset(Ref<TreeNode> node, const AssetHandle& handle) {
		if (!node) return;

		/* Push asset to node assets list */
		node->assets.push_back(handle);

		/* Get asset name */
		const Name& name = ProjectManagerHelpers::GetAssetName(node->assets.back());
		
		Vector<uint32_t>& vec = this->assetIndices[name];
		if (std::find(vec.begin(), vec.end(), node->id) == vec.end()) {
			vec.push_back(node->id);
		}
	}

	/**
	* Moves the specified asset
	* from node (directory)
	* 
	* @param from From which node
	* @param to To which node
	* @param name Asset name
	* 
	* @returns True if success
	*/
	bool 
	MoveAsset(Ref<TreeNode> from, Ref<TreeNode> to, const Name& name) {
		if (!from || !to) return false;

		for (Vector<AssetHandle>::iterator it = from->assets.begin(); it != from->assets.end(); it) {
			if (ProjectManagerHelpers::GetAssetName(*it) == name) {
				AssetHandle handle = *it;

				this->RemoveAsset(from, name);
				AddAsset(to, handle);

				return true;
			}
		}

		return false;
	}

	/**
	* Removes the specified asset
	* from the specified tree node
	* 
	* @param node Tree node
	* @param name Asset name
	*/
	void
	RemoveAsset(Ref<TreeNode> node, const Name& name) {
		if (!node) return;

		Vector<AssetHandle>& nodeAssets = node->assets;

		nodeAssets.erase(std::remove_if(nodeAssets.begin(), nodeAssets.end(),
			[&](AssetHandle& h) {
				bool bMatch = ProjectManagerHelpers::GetAssetName(h) == name;

				if (bMatch) {
					Vector<uint32_t>& vec = this->assetIndices[name];
					vec.erase(std::remove(vec.begin(), vec.end(), node->id), vec.end());
				}

				return bMatch;
			}
		), nodeAssets.end());
	}

	/**
	* Finds all the assets 
	* that have the specified name
	* 
	* @param name Asset name
	* 
	* @returns Found assets
	*/
	Vector<AssetHandle*>
	FindAssetsByName(const Name& name) {
		Vector<AssetHandle*> result;

		if (!this->assetIndices.contains(name)) return result;

		for (uint32_t nNodeID : this->assetIndices[name]) {
			Ref<TreeNode> node = this->nodeIndices[nNodeID];
			for (AssetHandle& handle : node->assets) {
				if (ProjectManagerHelpers::GetAssetName(handle) == name) {
					result.push_back(&handle);
				}
			}
		}

		return result;
	}

	/**
	* Finds the specified
	* asset on the specified node
	* 
	* @param node Finding node
	* @param name Asset name
	* 
	* @returns A pointer to the asset. nullptr if not found
	*/
	template<typename T>
	T*
	FindAssetInNode(Ref<TreeNode> node, const Name& name) {
		if (!node) return nullptr;

		/* Search asset in the node asset list */
		for (AssetHandle& handle : node->assets) {
			if (ProjectManagerHelpers::GetAssetName(handle) == name) {
				const AssetVariant& asset = AssetManager::GetInstance()->GetAsset(handle);
				if (T* ptr = std::get_if<T>(&asset)) {
					return ptr;
				}
			}
		}

		Logger::Warn("ProjectTree::FindAssetInNode: Asset {} not found", name);

		return nullptr;
	}

	/**
	* Find a node by relative path
	* 
	* @param current Current node
	* @param relPath Relative path
	* 
	* @returns Found tree node. nullptr if not found
	*/
	Ref<TreeNode> 
	FindNodeByRelativePath(
		const Ref<TreeNode>& current,
		const fs::path& relPath
	) {
		if (!current) return nullptr;

		if (current->dir.name == relPath.string())
			return current;

		for (auto& [id, child] : current->subNodes) {
			Ref<TreeNode> found = FindNodeByRelativePath(child, relPath);
			if (found) return found;
		}

		return nullptr;
	}
};

class ProjectManager {
public:
	using OnProjectOpenedCallback = std::function<void(const Project::Asset& projectAsset)>;

	ProjectManager();
	~ProjectManager() = default;

	bool OpenProject(const String& projectPath);
	
	static ProjectManager* GetInstance();

	bool 
	ProjectLoaded() {
		return this->m_projectDir.IsValid() 
			&& this->m_assetsDir.IsValid();
	}

	/**
	* Get all the assets 
	* on the project
	* 
	* @returns A map of the project directories 
	* with their assets
	*/
	ProjectTree GetProjectTree() const { return this->m_tree; }

	Vector<AssetHandle> GetNodeAssets(Ref<ProjectTree::TreeNode> node);

	Directory GetAssetsDir() { return this->m_assetsDir; }

	void
	SetOnProjectOpenedCallback(OnProjectOpenedCallback callback) {
		this->m_onProjectOpened = callback;
	}
private:
	OnProjectOpenedCallback m_onProjectOpened;

	ProjectTree m_tree;

	AssetManager* m_assetMgr;

	Directory m_projectDir;
	Directory m_assetsDir;

	static ProjectManager* m_instance;
};