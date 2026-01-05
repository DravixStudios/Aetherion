#pragma once
#include "Core/Renderer/DescriptorSet.h"
#include "Core/Renderer/Vulkan/VulkanDescriptorSet.h"
#include "Core/Renderer/Vulkan/VulkanDescriptorPool.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanDescriptorSet : public DescriptorSet {
public:
	using Ptr = Ref<VulkanDescriptorSet>;

	explicit VulkanDescriptorSet(VkDevice device);
	~VulkanDescriptorSet() override;

	void Allocate(Ref<DescriptorPool> pool, Ref<DescriptorSetLayout> layout) override;

	void WriteBuffer(uint32_t nBinding, uint32_t nArrayElement, const DescriptorBufferInfo& bufferInfo) override;
	void WriteTexture(uint32_t nBinding, uint32_t nArrayElement, const DescriptorImageInfo& imageInfo) override;

	void UpdateWrites() override;

	VkDescriptorSet GetVkSet() const { return this->m_descriptorSet; }

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanDescriptorSet>(device);
	}

private:
	VkDevice m_device;
	VkDescriptorSet m_descriptorSet;
	VkDescriptorPool m_pool;
};