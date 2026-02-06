#pragma once
#include "Core/Containers.h"

enum class EFenceFlags {
	SIGNALED
};

struct FenceCreateInfo {
	EFenceFlags flags;
};

class Fence {
public:
	static constexpr const char* CLASS_NAME = "Fence";

	using Ptr = Ref<Fence>;

	~Fence() = default;

	virtual void Create(const FenceCreateInfo& createInfo) = 0;
};