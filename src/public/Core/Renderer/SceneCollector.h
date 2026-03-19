#pragma once
#include "Core/Scene/Scene.h"
#include "Core/Renderer/MeshUploader.h"

#include "Utils.h"

class SceneCollector {
public:
	void 
	SetUploadedMeshes(const Map<String, UploadedMesh>* cache) {
		this->m_uploadedMeshes = cache;
	}

	CollectedDrawData Collect(Scene* scene);
private:
	const Map<String, UploadedMesh>* m_uploadedMeshes = nullptr;
};