#pragma once
#include <iostream>
#include <filesystem>

#include <tinyexr/tinyexr.h>

#include "Core/Containers.h"
#include "Utils.h"

inline bool 
LoadCubemap(const String filePath, void** ppData, uint32_t& nOutSize, uint32_t& nOutFaceSize) {
	float* pRGBA = nullptr;
	int nWidth, nHeight;
	const char* pcErr = nullptr;

	std::filesystem::path path = filePath;
	if (!path.is_absolute()) {
		path = std::filesystem::path(GetExecutableDir()) / filePath;
	}

	int nRet = LoadEXR(&pRGBA, &nWidth, &nHeight, path.string().c_str(), &pcErr);
	if (nRet != TINYEXR_SUCCESS) {
		if (pcErr) {
			Logger::Error("LoadCubemap: Error loading EXR: {}", pcErr);
			FreeEXRErrorMessage(pcErr);
		}
		
		return false;
	}

	int nFaceWidth = nWidth / 4;
	int nFaceHeight = nHeight / 3;

	int nFaceSize = nHeight / 2;
	int nCrossWidth = nFaceSize * 4;
	int nCrossHeight = nFaceSize * 3;

	Vector<float> crossData(nCrossWidth * nCrossHeight * 4);

	/* Convert equirectangular to horizontal cross */
	constexpr float PI = glm::pi<float>();

	struct Face {
		int nOffsetX, nOffsetY;
		int nFaceIndex;
	};

	Face faces[6] = {
		{ 2 * nFaceSize, 1 * nFaceSize, 0 }, // +X
		{ 0 * nFaceSize, 1 * nFaceSize, 1 }, // -X
		{ 1 * nFaceSize, 0 * nFaceSize, 2 }, // +Y
		{ 1 * nFaceSize, 2 * nFaceSize, 3 }, // -Y
		{ 1 * nFaceSize, 1 * nFaceSize, 4 }, // +Z
		{ 3 * nFaceSize, 1 * nFaceSize, 5 } // -Z
	};

	for (uint32_t f = 0; f < 6; f++) {
		int nBaseX = faces[f].nOffsetX;
		int nBaseY = faces[f].nOffsetY;
		int nFace = faces[f].nFaceIndex;

		for (uint32_t y = 0; y < nFaceSize; y++) {
			for (uint32_t x = 0; x < nFaceSize; x++) {
				/* UV in [-1, 1] */
				float u = (2.f * (x + .5f) / nFaceSize) - 1.f;
				float v = (2.f * (y + .5f) / nFaceSize) - 1.f;

				float dx, dy, dz;

				switch (nFace) {
					case 0: dx = 1; dy = -v; dz = -u; break; // +X
					case 1: dx = -1; dy = -v; dz = u; break; // -X
					case 2: dx = u; dy = 1; dz = v; break; // +Y
					case 3: dx = u; dy = -1; dz = -v; break; // -Y
					case 4: dx = u; dy = -v; dz = 1; break; // +Z
					case 5: dx = -u; dy = -v; dz = -1; break; // -Z
				}

				float len = std::sqrt(dx * dx + dy * dy + dz * dz);
				dx /= len; dy /= len; dz /= len;

				float theta = std::atan2(dz, dx); // [-PI, PI]
				float phi = std::asin(dy); // [-PI/2, PI/2]

				float srcU = (theta + PI) / (2.f * PI);
				float srcV = (PI * .5f - phi) / PI;

				int nSrcX = int(srcU * nWidth) % nWidth;
				int nSrcY = int(srcV * nHeight) % nHeight;

				int nSrcIdx = (nSrcY * nWidth + nSrcX) * 4;
				int nDstIdx = ((nBaseY + y) * (nFaceSize * 4) + (nBaseX + x)) * 4;

				crossData[nDstIdx + 0] = pRGBA[nSrcIdx + 0];
				crossData[nDstIdx + 1] = pRGBA[nSrcIdx + 1];
				crossData[nDstIdx + 2] = pRGBA[nSrcIdx + 2];
				crossData[nDstIdx + 3] = pRGBA[nSrcIdx + 3];
			}
		}
	}

	free(pRGBA);

	/* Extract all 6 faces */
	uint32_t nFacePixels = nFaceSize * nFaceSize * 4;
	uint32_t nTotalPixels = nFacePixels * 6;

	Vector<float> extractedData(nTotalPixels);

	/*
		Layout (horizontal cross):
			[  ][+Y][  ][  ]
			[-X][+Z][+X][-Z]
			[  ][-Y][  ][  ]
	*/

	struct FaceOffset {
		int x;
		int y;
	};

	Vector<FaceOffset> offsets = {
		{ 2 * nFaceSize, nFaceSize }, // +X
		{ 0, nFaceSize }, // -X
		{ nFaceSize, 0 }, // +Y
		{ nFaceSize, 2 * nFaceSize }, // -Y
		{ nFaceSize, nFaceSize }, // +Z
		{ 3 * nFaceSize, nFaceSize } // -Z
	};

	/* Copy each face */
	const float* pcSrcRGBA = crossData.data();
	float* pDstData = extractedData.data();

	for (int nFace = 0; nFace < 6; nFace++) {
		float* pDstFace = pDstData + (nFace * nFaceSize * nFaceSize * 4);

		for (uint32_t y = 0; y < nFaceSize; y++) {
			for (uint32_t x = 0; x < nFaceSize; x++) {
				int nSrcX = offsets[nFace].x + x;
				int nSrcY = offsets[nFace].y + y;
				int nSrcIdx = (nSrcY * nCrossWidth + nSrcX) * 4;
				int nDstIdx = (y * nFaceSize + x) * 4;

				for (int c = 0; c < 4; c++) {
					pDstFace[nDstIdx + c] = pcSrcRGBA[nSrcIdx + c];
				}
			}
		}
	}

	uint32_t nFaceByteSize = nFacePixels * sizeof(float);
	uint32_t nTotalByteSize = nTotalPixels * sizeof(float);
	
	void* pOut = malloc(nTotalByteSize);
	if (!pOut) {
		Logger::Error("LoadCubemap:: Out of memory");
		return false;
	}

	memcpy(pOut, extractedData.data(), nTotalByteSize);

	*ppData = pOut;
	nOutSize = nTotalByteSize;
	nOutFaceSize = nFaceSize;
}