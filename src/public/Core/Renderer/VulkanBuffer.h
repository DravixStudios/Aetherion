#pragma once
#include "Core/Renderer/GPUBuffer.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanBuffer : public GPUBuffer {
public:
	VulkanBuffer();
};