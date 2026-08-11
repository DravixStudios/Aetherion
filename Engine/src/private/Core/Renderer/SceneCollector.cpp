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

	result.viewProj = view * proj;
	result.view = view;
	result.proj = proj;
	result.cameraPosition = glm::vec3(cam->transform.location.x, cam->transform.location.y, cam->transform.location.z);

	Map<String, GameObject*> gameObjects;
	Ref<Hierarchy::HierarchyNode> rootNode = scene->GetHierarchy().root;
	this->CollectGameObjects(rootNode, gameObjects);

	for (auto& [name, gameObject] : gameObjects) {
		/* Find a mesh component on the current GameObject */
		Map<String, Component*> components = gameObject->GetComponents();
		Map<String, Component*>::iterator it = components.find("MeshComponent");
		if (it == components.end()) continue;

		Mesh* mesh = dynamic_cast<Mesh*>(it->second);
		if (!mesh || !mesh->IsLoaded()) continue;

		/* Check if the mesh is on the uploaded meshes cache */
		const String& meshName = mesh->GetMeshData().name;
		Map<String, UploadedMesh>::const_iterator uploadIt = this->m_uploadedMeshes->find(meshName);

		if (uploadIt == this->m_uploadedMeshes->end()) continue;

		const UploadedMesh& uploadedMesh = uploadIt->second;
		glm::mat4 world = gameObject->transform.GetWorldMatrix();

		for (auto& [idx, subMesh] : uploadedMesh.subMeshes) {
			uint32_t nWvpIdx = static_cast<uint32_t>(result.wvps.size());
			uint32_t nMaterialIdx = static_cast<uint32_t>(result.materials.size());

			WVP wvp = { };
			wvp.World = world;
			wvp.View = view;
			wvp.Projection = proj;

			result.wvps.push_back(wvp);

			UploadedSubMeshMaterial uploadedMaterial = subMesh.material;

			MaterialInstanceData material = { };
			material.albedoIndex = uploadedMaterial.nAlbedoIndex;
			material.ormIndex = uploadedMaterial.nORMIndex;
			material.emissiveIndex = uploadedMaterial.nEmissiveIndex;
			material.normalIndex = uploadedMaterial.nNormalIndex;

			material.albedoColor = uploadedMaterial.albedoColor;
			material.emissiveColor = uploadedMaterial.emissiveColor;
			material.ao = uploadedMaterial.ao;
			material.roughness = uploadedMaterial.roughness;
			material.metallic = uploadedMaterial.metallic;
			material.materialFlags = uploadedMaterial.materialFlags;

			result.materials.push_back(material);

			ObjectInstanceData instance = { };
			instance.wvpOffset = nWvpIdx * sizeof(WVP);
			instance.materialOffset = nMaterialIdx * sizeof(MaterialInstanceData);

			result.instances.push_back(instance);

			DrawBatch batch = { };
			batch.indexCount = subMesh.geometry.nIndexCount;
			batch.firstIndex = subMesh.geometry.nFirstIndex;
			batch.vertexOffset = subMesh.geometry.nVertexOffset;
			batch.instanceDataIndex = static_cast<uint32_t>(result.instances.size() - 1);
			batch.nBlockIdx = subMesh.nBlockIdx;

			result.batches.push_back(batch);
		}
	}

	result.nTotalBatches = static_cast<uint32_t>(result.batches.size());

	return result;
}

void 
SceneCollector::CollectGameObjects(Ref<Hierarchy::HierarchyNode> node, Map<String, GameObject*>& outObjects) {
	if (node->pObj != nullptr)
		outObjects[String(node->name)] = node->pObj;

	if (!node->HasChildren())
		return;

	for(Ref<Hierarchy::HierarchyNode> children : node->children)
		this->CollectGameObjects(children, outObjects);
}