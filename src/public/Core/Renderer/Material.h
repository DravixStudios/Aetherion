#pragma once
#include "Core/Renderer/Shader.h"
#include "Core/Resources/MaterialAsset.h"

struct MaterialCreateInfo {
	Vector4 albedoColor;
	Vector4 emissiveColor;
	float ao = 1.f;
	float roughness = .5f;
	float metallic = .5f;
};

class Material {
public:
	explicit Material() = default;
	~Material() = default;

	void Create(const MaterialCreateInfo& createInfo);
	void Create(const MaterialAsset& asset);

	/**
	* Get Material flags
	* 
	* @returns Material flags
	*/
	EMaterialFlags GetFlags() const { return this->m_flags; }

	/**
	* Check if Material has specified flag
	* 
	* @param flag Flag to find
	* 
	* @returns True if flag found
	*/
	bool
	HasFlag(const EMaterialFlags& flag) {
		if ((this->m_flags & flag) != static_cast<EMaterialFlags>(0)) {
			return true;
		}

		return false;
	}

	AssetHandle m_albedoHandle;
	AssetHandle m_ormHandle;
	AssetHandle m_emissiveHandle;
	AssetHandle m_normalHandle;

	Vector4 m_albedo;
	Vector4 m_emissiveColor;
	float m_ao = 1.f;
	float m_roughness = .5f;
	float m_metallic = .5f;
private:
	EMaterialFlags m_flags;

	MaterialAsset m_materialAsset;
};