#pragma once
#include "Core/Renderer/Device.h"
#include <optional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct QueueFamilyIndices;

class VulkanDevice : public Device {
public:
	explicit VulkanDevice(VkPhysicalDevice physicalDevice, VkInstance instance, VkSurfaceKHR surface = VK_NULL_HANDLE);
	~VulkanDevice() override;

	void Create(const DeviceCreateInfo& createInfo) override;
	void WaitIdle() override;

	QueueFamilyInfo GetQueueFamilyInfo(EQueueType queueType) const override;

	uint32_t FindQueueFamily(
		bool bGraphics = false,
		bool bCompute = false,
		bool bTransfer = false,
		bool bPresent = false
	) const override;

	void GetLimits(
		uint32_t& nMaxUniformBufferRange,
		uint32_t& nMaxStorageBufferRange,
		uint32_t& nMaxPushConstantSize,
		uint32_t& nMaxBoundDescriptorSets
	) const override;

	const char* GetDeviceName() const override;
private:
	VkPhysicalDevice m_physicalDevice;
	VkInstance m_instance;
	VkSurfaceKHR m_surface;
	VkDevice m_device;
};