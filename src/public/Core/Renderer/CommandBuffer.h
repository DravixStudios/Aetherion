#pragma once
#include "Utils.h"

class CommandBuffer {
public:
	static constexpr const char* CLASS_NAME = "CommandBuffer";

	using Ptr = Ref<CommandBuffer>;

	virtual ~CommandBuffer() = default;

	/**
	* Begins a command buffer
	*/
	virtual void Begin() = 0;

	/**
	* Ends a command buffer
	*/
	virtual void End() = 0;

	/**
	* Resets a command buffer
	*/
	virtual void Reset() = 0;
};