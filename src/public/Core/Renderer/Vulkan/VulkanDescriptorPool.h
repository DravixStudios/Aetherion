#pragma once
#include "Core/Renderer/DescriptorPool.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Core/Renderer/Vulkan/VulkanHelpers.h"
#include "Utils.h"

class VulkanDescriptorPool : public DescriptorPool {
public:
	using Ptr = Ref<VulkanDescriptorPool>;

	explicit VulkanDescriptorPool(VkDevice device);
	~VulkanDescriptorPool() override;

	void Create(const DescriptorPoolCreateInfo& createInfo) override;
	void Reset() override;

	VkDescriptorPool GetVkPool() const { return this->m_pool; }
	
	/* Factory for creating a shared_ptr of VulkanDescriptorPool */
	static Ptr 
	CreateShared(VkDevice device) {
		return CreateRef<VulkanDescriptorPool>(device);
	}
private:
	VkDevice m_device;
	VkDescriptorPool m_pool;
};