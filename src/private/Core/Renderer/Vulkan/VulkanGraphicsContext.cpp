#include "Core/Renderer/Vulkan/VulkanGraphicsContext.h"

VulkanGraphicsContext::VulkanGraphicsContext(VkCommandBuffer commandBuffer) 
	: m_commandBuffer(commandBuffer), m_currentPipeline(VK_NULL_HANDLE), 
	m_currentBindPoint(VK_PIPELINE_BIND_POINT_MAX_ENUM) {}


