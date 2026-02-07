#pragma once
#include "Utils.h"
#include <map>

struct TextureData {
	String name;
	Vector<unsigned char> data;
	uint32_t nWidth = 0;
	uint32_t nHeight = 0;
	bool bCompressed = false;
};

struct SubMeshData {
	Vector<Vertex> vertices;
	Vector<uint16_t> indices;
	TextureData albedo;
	TextureData orm;
	TextureData emissive;
};

struct MeshData {
	String name;
	std::map<uint32_t, SubMeshData> subMeshes;
	bool bLoaded = false;
};