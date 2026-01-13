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
	static constexpr const char* CLASS_NAME = "Device";

	using Ptr = Ref<Device>;

	virtual ~Device();

	/**
	* Creates a logical device
	* 
	* @param createInfo Device create info
	*/
	virtual void Create(const DeviceCreateInfo& createInfo) = 0;

	/**
	* Waits till device finishes all the operations
	*/
	virtual void WaitIdle() = 0;

	/**
	* Gets queue family information by type
	* 
	* @param queueType Queue type
	* 
	* @returns Queue family info
	*/
	virtual QueueFamilyInfo GetQueueFamilyInfo(EQueueType queueType) const = 0;
	
	/**
	*	Gets a queue family index that supports the specified operations
	* 
	* @param bGraphics Needs graphics support
	* @param bCompute Needs compute support
	* @param bTransfer Needs transfer support
	* @param bPresent Needs present support
	*/
	virtual uint32_t FindQueueFamily(
		bool bGraphics = false,
		bool bCompute = false,
		bool bTransfer = false,
		bool bPresent = false
	) const = 0;

	/**
	* Gets device limits
	*/
	virtual void GetLimits(
		uint32_t& nMaxUniformBufferRange,
		uint32_t& nMaxStorageBufferRange,
		uint32_t& nMaxPushContantsSize,
		uint32_t& nMaxBoundDescriptorSets
	) const = 0;

	/**
	* Device name
	*/
	virtual const char* GetDeviceName() const = 0;
};