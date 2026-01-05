#include "Core/Renderer/Vulkan/VulkanDescriptorPool.h"

VulkanDescriptorPool::VulkanDescriptorPool(VkDevice device) 
	: DescriptorPool::DescriptorPool(), m_device(device), m_pool(VK_NULL_HANDLE) {}