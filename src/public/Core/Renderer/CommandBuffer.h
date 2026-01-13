#pragma once
#include "Utils.h"

class CommandBuffer {
public:
	using Ptr = Ref<CommandBuffer>;

	virtual ~CommandBuffer() = default;

	virtual void Begin() = 0;
	virtual void End() = 0;
	virtual void Reset() = 0;
};