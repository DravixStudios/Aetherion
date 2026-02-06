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

	virtual ~Fence() = default;

	/**
	* Creates a fence
	* 
	* @param createInfo Fence create info
	*/
	virtual void Create(const FenceCreateInfo& createInfo) = 0;

	/**
	* Resets the fence
	*/
	virtual void Reset() = 0;
};