#pragma once
#include "Core/Containers.h"

class Semaphore {
public:
	static constexpr const char* CLASS_NAME = "Semaphore";

	using Ptr = Ref<Semaphore>;

	virtual ~Semaphore() = default;

	/**
	* Creates a semaphore
	*/
	virtual void Create() = 0;
};