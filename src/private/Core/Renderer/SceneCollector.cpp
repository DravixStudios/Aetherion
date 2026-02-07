#include "Core/Renderer/SceneCollector.h"

/**
* Collects scene draw data
* 
* @param scene Scene
* 
* @returns Collected draw data from scene
*/
CollectedDrawData
SceneCollector::Collect(Scene* scene) {
	CollectedDrawData result = { };

	if (!scene || !this->m_uploadedMeshes) return result;

	/* Get the current camera's View and Projection */
	Camera* cam = scene->GetCurrentCamera();
	glm::mat4 view = cam->GetView();
	glm::mat4 proj = cam->GetProjection();

	for (auto& [name, gameObject] : scene->GetObjects()) {
		/* Find a mesh component on the current GameObject */
		std::map<String, Component*> components = gameObject->GetComponents();
		std::map<String, Component*>::iterator it = components.find("Mesh");
		if (it == components.end()) continue;

		Mesh* mesh = dynamic_cast<Mesh*>(it->second);
		if (!mesh || !mesh->IsLoaded()) continue;

		/* Check if the mesh is on the uploaded meshes cache */
		const String& meshName = mesh->GetMeshData().name;
		std::map<String, UploadedMesh>::const_iterator uploadIt = this->m_uploadedMeshes->find(meshName);

		if (uploadIt == this->m_uploadedMeshes->end()) continue;

		const UploadedMesh& uploadedMesh = uploadIt->second;
		glm::mat4 world = gameObject->transform.GetWorldMatrix();

		for (auto& [idx, subMesh] : uploadedMesh.subMeshes) {
			uint32_t nWvpIdx = static_cast<uint32_t>(result.wvps.size());

			WVP wvp = { };
			wvp.World = world;
			wvp.View = view;
			wvp.Projection = proj;

			ObjectInstanceData instance = { };
			instance.wvpOffset = nWvpIdx;
			instance.textureIndex = subMesh.nAlbedoIndex;
			instance.ormTextureIndex = subMesh.nORMIndex;
			instance.emissiveTextureIndex = subMesh.nEmissiveIndex;

			result.instances.push_back(instance);

			DrawBatch batch = { };
			batch.indexCount = subMesh.geometry.nIndexCount;
			batch.firstIndex = subMesh.geometry.nFirstIndex;
			batch.vertexOffset = subMesh.geometry.nVertexOffset;
			batch.instanceDataIndex = static_cast<uint32_t>(result.instances.size() - 1);

			result.batches.push_back(batch);
		}
	}

	result.nTotalBatches = static_cast<uint32_t>(result.batches.size());

	return result;
}