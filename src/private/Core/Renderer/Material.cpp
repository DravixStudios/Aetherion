#include "Core/Renderer/Material.h"

/**
* Create material
* 
* @param createInfo Material create info
*/
void 
Material::Create(const MaterialCreateInfo& createInfo) {
	this->m_albedo = createInfo.albedoColor;
	this->m_emissiveColor = createInfo.emissiveColor;
	this->m_ao = createInfo.ao;
	this->m_roughness = createInfo.roughness;
	this->m_metallic = createInfo.metallic;
}

/**
* Initialize material from a Material asset
* 
* @param asset Material asset
*/
void 
Material::Create(const MaterialAsset& asset) {
	this->m_materialAsset = asset;
	this->m_flags = asset.header.flags;

	this->m_albedoHandle = asset.albedoHandle;
	this->m_ormHandle = asset.ormHandle;
	this->m_emissiveHandle = asset.emissiveHandle;
	this->m_normalHandle = asset.normalHandle;

	this->m_albedo = asset.albedo;
	this->m_emissiveColor = asset.emissiveColor;
	this->m_ao = asset.ao;
	this->m_roughness = asset.roughness;
	this->m_metallic = asset.metallic;
}