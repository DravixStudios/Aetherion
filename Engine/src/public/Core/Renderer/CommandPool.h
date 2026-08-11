#pragma once
#include <cstdint>

#include "Core/Containers.h"
#include "Core/Renderer/CommandBuffer.h"

enum class ECommandPoolFlags : uint32_t {
	NONE = 0,
	TRANSIENT = 1 << 0,
	RESET_COMMAND_BUFFER = 1 << 2,
	ALL = 0x7FFFFFFF
};

inline ECommandPoolFlags 
operator|(ECommandPoolFlags a, ECommandPoolFlags b) {
	return static_cast<ECommandPoolFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ECommandPoolFlags 
operator&(ECommandPoolFlags a, ECommandPoolFlags b) {
	return static_cast<ECommandPoolFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
struct CommandPoolCreateInfo {
	uint32_t nQueueFamilyIndex;
	ECommandPoolFlags flags = ECommandPoolFlags::NONE;
};

class CommandPool {
public:
	static constexpr const char* CLASS_NAME = "CommandPool";

	using Ptr = Ref<CommandPool>;

	virtual ~CommandPool() = default;

	/**
	* Creates a command pool
	* 
	* @param createInfo Command pool create info
	*/
	virtual void Create(const CommandPoolCreateInfo& createInfo) = 0;

	/**
	* Allocates a command buffer from the current pool
	* 
	* @returns A new command buffer
	*/
	virtual Ref<CommandBuffer> AllocateCommandBuffer() = 0;

	/**
	* Allocates many command buffers from the current pool
	* 
	* @param nCount Command buffer count
	* 
	* @returns A vector of command buffers
	*/
	virtual Vector<Ref<CommandBuffer>> AllocateCommandBuffers(uint32_t nCount) = 0;

	/**
	* Frees a command buffer
	* 
	* @param commandBuffer Command buffer to free
	*/
	virtual void FreeCommandBuffer(Ref<CommandBuffer> commandBuffer) = 0;

	/**
	* Frees multiple command buffers
	*
	* @param commandBuffers A vector of command buffers to free
	*/
	virtual void FreeCommandBuffers(const Vector<Ref<CommandBuffer>>& commandBuffers) = 0;

	/**
	* Resets the command pool freeing all the command buffers
	* 
	* @param bReleaseResources If true, release the resources
	*/
	virtual void Reset(bool bReleaseResources = false) = 0;
};