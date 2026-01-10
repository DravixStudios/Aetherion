#pragma once

class CommandBuffer {
public:
	virtual ~CommandBuffer() = default;

	virtual void Begin() = 0;
	virtual void End() = 0;
	virtual void Reset() = 0;
};