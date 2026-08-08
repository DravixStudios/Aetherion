#pragma once
#include "Core/Containers.h"

class CommandBuffer;

struct UploadContext {
	using Ptr = Ref<UploadContext>;
	Ref<CommandBuffer> commandBuffer;
};