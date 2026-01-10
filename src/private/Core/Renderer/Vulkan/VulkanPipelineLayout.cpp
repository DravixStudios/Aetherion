#include "Core/Renderer/Vulkan/VulkanPipelineLayout.h"

VulkanPipelineLayout::VulkanPipelineLayout(VkDevice device) 
	: m_device(device), m_layout(VK_NULL_HANDLE) {}

