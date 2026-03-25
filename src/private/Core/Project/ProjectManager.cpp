#include "Core/Project/ProjectManager.h"
#include <fstream>
#include <algorithm>

String
SplitPath(const String& path, const String& delim) {
	Vector<String> parts;

	/* Convert path from string to filesystem path */
	std::filesystem::path fsPath(path);

	/* Split path in parts */
	for (const auto& part : fsPath) {
		parts.push_back(part.string());
	}

	Vector<String>::iterator it = std::find(parts.begin(), parts.end(), delim);
	
	/* Erase everything before the delimitator */
	if (it != parts.end()) {
		parts = Vector<String>(it, parts.end());
	}

	/* Remove the last part (filename) */
	if (!parts.empty()) {
		parts.pop_back();
	}

	/* Reconstruct the path */
	std::filesystem::path result;
	for (const String& part : parts) {
		result /= part;
	}

	return result.string();
}

ProjectManager* ProjectManager::m_instance;

ProjectManager::ProjectManager() : m_assetMgr(nullptr) { }

/**
* Open a project
* 
* @param projectPath Project path
* 
* @returns True if success
*/
bool
ProjectManager::OpenProject(const String& projectPath) {
	if (!this->m_assetMgr) this->m_assetMgr = AssetManager::GetInstance();

	Directory projectDir = { };
	projectDir.name = projectPath;

	/* Check if project directory exists */
	if (!projectDir.Exists()) {
		Logger::Error("ProjectManager::OpenProject: Project directory does not exist: {}", projectPath);
		return false;
	}

	/* Check if project directory is valid */
	if (!projectDir.IsValid()) {
		Logger::Error("ProjectManager::OpenProject: Invalid project directory: {}", projectPath);
		return false;
	}

	/* Check if assets directory exists */
	Directory assetsDir = { };
	fs::path assetsPath = fs::path(projectPath) / "Assets";
	assetsDir.name = assetsPath.string();

	if (!assetsDir.Exists()) {
		Logger::Error("ProjectManager::OpenProject: Assets directory does not exist");
		return false;
	}

	/* Check if assets directory is valid */
	if (!assetsDir.IsValid()) {
		Logger::Error("ProjectManager::OpenProject: Invalid Assets directory");
		return false;
	}

	/* Initialize project tree */
	this->m_tree = ProjectTree::Create(assetsDir);

	/* Store project required directories */
	this->m_assetsDir = assetsDir;
	this->m_projectDir = projectDir;

	/* 
		Search for all .aeth files 
		and load asset infos.

		If found a directory, create
		a node and add it to the 
		project tree
	*/
	for (const auto& entry : fs::recursive_directory_iterator(assetsPath)) {
		/*
			If current entry is a directory,
			create a node and add it to the
			project tree
		*/
		if (entry.is_directory() && !entry.is_regular_file()) {
			fs::path relPath = fs::relative(entry.path(), assetsPath);

			Ref<ProjectTree::TreeNode> parentNode = this->m_tree.root;
			fs::path buildPath = assetsPath;

			for (const auto& part : relPath) {
				buildPath /= part;

				Ref<ProjectTree::TreeNode> existing = this->m_tree.FindNodeByRelativePath(this->m_tree.root, buildPath);

				if (existing) {
					parentNode = existing;
					continue;
				}

				Directory dir = { };
				dir.name = buildPath.string();

				/* Check if valid directory */
				if (!dir.IsValid() || !dir.Exists()) continue;

				Ref<ProjectTree::TreeNode> newNode = this->m_tree.CreateNode(dir, parentNode);
				parentNode = newNode;
			}

			continue;
		}

		/* If is not a regular file or directory */
		if (!entry.is_regular_file()) {
			continue;
		}

		/* Check file extension */
		if (entry.path().extension() != ".aeth")
			continue;

		/* Check if file is open */
		std::ifstream file(entry.path(), std::ios::binary);
		if (!file.is_open())
			continue;

		/* Read magic number, version and raw asset type */
		uint32_t nMagic = 0;
		uint32_t nVersion = 0;
		uint32_t nRawType = 0;
		
		file.read(reinterpret_cast<char*>(&nMagic), sizeof(nMagic));
		file.read(reinterpret_cast<char*>(&nVersion), sizeof(nVersion));
		file.read(reinterpret_cast<char*>(&nRawType), sizeof(nRawType));

		/* 
			Check if magic number is our AETH magic 
			We do this just for filtering valid .aeth
			files, AssetManager will do the same.
		*/
		if (nMagic != MAGIC_NUMBER) {
			file.close();
			continue;
		}

		EAssetType type = static_cast<EAssetType>(nRawType);
		file.close();

		/* Get the current asset path */
		String assetPath = SplitPath(entry.path().string(), "Assets");
		String fullPath = entry.path().string();

		/* Find asset node */
		fs::path relPath = fs::relative(entry.path().parent_path(), assetsPath);

		Ref<ProjectTree::TreeNode> node = this->m_tree.root;
		fs::path buildPath = assetsPath;

		for (const auto& part : relPath) {
			buildPath /= part;

			Ref<ProjectTree::TreeNode> existing = this->m_tree.FindNodeByRelativePath(this->m_tree.root, buildPath);

			if (existing) {
				node = existing;
			}
		}

		/* Read file with AssetManager depending on it's type */
		switch (type) {
		case EAssetType::MESH:
			{
				MeshAsset meshAsset = this->m_assetMgr->ReadMesh(fullPath);
				this->m_tree.AddAsset(node, meshAsset);
			}

			break;
		case EAssetType::TEXTURE:
			break;
		}
	}

	return true;
}


/**
* Get all the assets on the
* specified project tree node
*
* @param node Tree node
*
* @returns The assets on the node
*/
Vector<AssetVariant> 
ProjectManager::GetNodeAssets(Ref<ProjectTree::TreeNode> node) {
	/* Check if directory exists */
	if(!node->dir.Exists()) {
		Logger::Error("ProjectManager::GetNodeAssets: Node directory doesn't exist: {}", node->dir.name);
		return Vector<AssetVariant>();
	}

	/* Check if directory is valid */
	if(!node->dir.IsValid()) {
		Logger::Error("ProjectManager::GetNodeAssets: Node directory is not valid: {}", node->dir.name);
		return Vector<AssetVariant>();
	}

	Vector<AssetVariant> assets = node->assets;

	return assets;
}

ProjectManager*
ProjectManager::GetInstance() {
	if (ProjectManager::m_instance == nullptr) {
		ProjectManager::m_instance = new ProjectManager();
	}

	return ProjectManager::m_instance;
}