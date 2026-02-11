#pragma once
#include "Core/Scene/Scene.h"
#include "Core/Renderer/MeshUploader.h"

#include "Utils.h"

class SceneCollector {
public:
	void 
	SetUploadedMeshes(const std::map<String, UploadedMesh>* cache) {
		this->m_uploadedMeshes = cache;
	}

	CollectedDrawData Collect(Scene* scene);
private:
	const std::map<String, UploadedMesh>* m_uploadedMeshes = nullptr;
};