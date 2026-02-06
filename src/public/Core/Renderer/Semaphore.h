#pragma once
#include "Core/Containers.h"

enum class ESemaphoreType {
	BINARY,
	TIMELINE
};

struct SemaphoreCreateInfo {
	ESemaphoreType type;
	uint64_t nInitialValue = 0;
};

class Semaphore {
public:
	static constexpr const char* CLASS_NAME = "Semaphore";

	using Ptr = Ref<Semaphore>;

	virtual ~Semaphore() = default;

	virtual void Create(const SemaphoreCreateInfo& createInfo) = 0;
};