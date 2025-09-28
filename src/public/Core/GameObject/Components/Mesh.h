#pragma once
#include "Core/GameObject/Components/Component.h"
#include "Utils.h"

#include <vector>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

/* Forward declarations */
class Core;

class Mesh : public Component {
public:
	Mesh(std::string name);

	void Start() override;
	void Update() override;

	bool LoadModel(std::string filePath);
	
private:
	bool m_bMeshImported;

	Core* m_core;
};