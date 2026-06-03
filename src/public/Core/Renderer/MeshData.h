#pragma once
#include "Utils.h"

#include "Core/Containers.h"
#include "Core/Renderer/Material.h"

#include "Math/Vector3.h"

struct TextureData {
	String name;
	Vector<Byte> data;
	uint32_t nWidth = 0;
	uint32_t nHeight = 0;
	bool bCompressed = false;
};

struct SubMeshData {
	Vector<Vertex> vertices;
	Vector<uint32_t> indices;

	/**
	* Check if SubMes material data has specified flag
	* 
	* @param flag Material flag to check
	* 
	* @returns True if it has
	*/
	EMaterialFlags materialFlags;
	bool HasMaterialFlag(EMaterialFlags flag) {
		if ((this->materialFlags & flag) != static_cast<EMaterialFlags>(0)) {
			return true;
		}

		return false;
	}

	/* Material data */
	Vector4 albedoColor = Vector4{ 1.f, 1.f, 1.f, 1.f };
	float ao = 1.f;
	float roughness = .5f;
	float metallic = .5f;
	Vector4 emissiveColor = Vector4{ 0.f, 0.f, 0.f, 1.f };

	TextureData albedo;
	TextureData orm;
	TextureData emissive;
	TextureData normal;
};

struct MeshData {
	String name;
	Map<uint32_t, SubMeshData> subMeshes;
	bool bLoaded = false;
};
