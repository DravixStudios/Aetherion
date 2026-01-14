#pragma once
#include <optional>
#include <set>

#include "Utils.h"
#include "Core/Logger.h"

#include "Core/Renderer/Device.h"

#include "Core/Renderer/Vulkan/VulkanRenderer.h"
#include "Core/Renderer/Vulkan/VulkanCommandPool.h"
#include "Core/Renderer/Vulkan/VulkanCommandBuffer.h"
#include "Core/Renderer/Vulkan/VulkanGraphicsContext.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanDevice : public Device {
public:
	using Ptr = Ref<VulkanDevice>;

	explicit VulkanDevice(VkPhysicalDevice physicalDevice, VkInstance instance, VkSurfaceKHR surface = VK_NULL_HANDLE);
	~VulkanDevice() override;

	void Create(const DeviceCreateInfo& createInfo) override;
	void WaitIdle() override;

	Ref<CommandPool> CreateCommandPool(const CommandPoolCreateInfo& createInfo) override;
	Ref<GraphicsContext> CreateContext(Ref<CommandPool>& commandPool) override;

	void GetLimits(
		uint32_t& nMaxUniformBufferRange,
		uint32_t& nMaxStorageBufferRange,
		uint32_t& nMaxPushConstantSize,
		uint32_t& nMaxBoundDescriptorSets
	) const override;

	const char* GetDeviceName() const override;

	static Ptr
	CreateShared(VkPhysicalDevice physicalDevice, VkInstance instance, VkSurfaceKHR surface = VK_NULL_HANDLE) {
		return CreateRef<VulkanDevice>(physicalDevice, instance, surface);
	}
private:
	VkPhysicalDevice m_physicalDevice;
	VkInstance m_instance;
	VkSurfaceKHR m_surface;
	VkDevice m_device;

	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;
	
	Vector<VkQueueFamilyProperties> m_queueFamilyProperties;
	void CacheQueueFamilyProperties();

	QueueFamilyIndices FindQueueFamilies();

	VkPhysicalDeviceProperties m_devProperties;

	void CachePhysicalDeviceProperties();

};