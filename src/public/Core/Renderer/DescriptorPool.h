#pragma once
#include "Core/Renderer/DescriptorSetLayout.h"

struct DescriptorPoolSize {
	EDescriptorType type;
	uint32_t nDescriptorCount;
};

struct DescriptorPoolCreateInfo {
	uint32_t nMaxSets;
	Vector<DescriptorPoolSize> poolSizes;
	bool bUpdateAfterBind = false;
};

class DescriptorPool {
public:
	DescriptorPool() = default;
	virtual ~DescriptorPool() = default;

	virtual void Create(const DescriptorPoolCreateInfo& createInfo) = 0;
	virtual void Reset() = 0;
};