#pragma once
#include <cstdint>

#include "Core/Containers.h"

enum class EQueueType {
	GRAPHICS,
	COMPUTE,
	TRANSFER,
	PRESENT
};

struct QueueFamilyInfo {
	uint32_t nFamilyIndex = UINT32_MAX;
	uint32_t nQueueCount = 0;
	bool bSupportsGraphics = false;
	bool bSupportsCompute = false;
	bool bSupportsTransfer = false;
	bool bSupportsPresent = false;
};

struct DeviceCreateInfo {
	Vector<const char*> requiredExtensions;
	bool bEnableGeometryShader = false;
	bool bEnableTessellationShader = false;
	bool bEnableSamplerAnisotroply = false;
	bool bEnableMultiDrawIndirect = false;
	bool bEnableDepthClamp = false;
	Vector<const char*> validationLayers; // DEBUG ONLY
};

class Device {
public:
	virtual ~Device();


};