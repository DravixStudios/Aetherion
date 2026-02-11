#pragma once
#include "Core/Renderer/Semaphore.h"

#include "Core/Renderer/Vulkan/VulkanDevice.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanSemaphore : public Semaphore {
public:
	using Ptr = Ref<VulkanSemaphore>;

	explicit VulkanSemaphore(Ref<VulkanDevice> device);
	~VulkanSemaphore() override;

	void Create() override;

	VkSemaphore GetVkSemaphore() const { return this->m_semaphore; }

	static Ptr
	CreateShared(Ref<VulkanDevice> device) {
		return CreateRef<VulkanSemaphore>(device);
	}

private:
	Ref<VulkanDevice> m_device;

	VkSemaphore m_semaphore;
};