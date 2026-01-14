#pragma once
#include <cstdint>

#include "Core/Containers.h"
#include "Core/Renderer/CommandPool.h"

enum class EQueueType {
	GRAPHICS,
	COMPUTE,
	TRANSFER,
	PRESENT
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
	* Creates a command pool
	* 
	* @param createInfo Command pool create info
	*
	* @returns Created command pool
	*/
	virtual Ref<CommandPool> CreateCommandPool(const CommandPoolCreateInfo& createInfo);

	/**
	* Waits till device finishes all the operations
	*/
	virtual void WaitIdle() = 0;

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
	* Gets the Device name
	* 
	* @returns Device name
	*/
	virtual const char* GetDeviceName() const = 0;
};