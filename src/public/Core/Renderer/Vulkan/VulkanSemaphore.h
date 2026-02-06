#pragma once
#include "Core/Renderer/Semaphore.h"
#include "Core/Renderer/Device.h"

class VulkanSemaphore : public Semaphore {
public:
	explicit VulkanSemaphore(Ref<Device> device);
	~VulkanSemaphore() override;

	void Create(const SemaphoreCreateInfo& createInfo) override;

	VkSemaphore GetVkSemaphore() const { return this->m_semaphore; }

private:
	Ref<Device> m_device;

	VkSemaphore m_semaphore;
};