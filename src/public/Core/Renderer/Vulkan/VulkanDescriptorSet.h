#pragma once
#include "Core/Renderer/DescriptorSet.h"

#include "Core/Renderer/Vulkan/VulkanDescriptorSetLayout.h"
#include "Core/Renderer/Vulkan/VulkanDescriptorPool.h"
#include "Core/Renderer/Vulkan/VulkanDescriptorSet.h"

#include "Core/Renderer/Vulkan/VulkanBuffer.h"
#include "Core/Renderer/Vulkan/VulkanTexture.h"
#include "Core/Renderer/Vulkan/VulkanImageView.h"
#include "Core/Renderer/Vulkan/VulkanSampler.h"

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

	void WriteBuffers(
		uint32_t nBinding, 
		uint32_t nFirstArrayElement,
		const Vector<DescriptorBufferInfo>& bufferInfos,
		EBufferType bufferType = EBufferType::UNIFORM_BUFFER
	) override;
	void WriteTextures(uint32_t nBinding, uint32_t nFirstArrayElement, const Vector<DescriptorImageInfo>& imageInfos) override;

	void UpdateWrites() override;

	VkDescriptorSet GetVkSet() const { return this->m_descriptorSet; }

	Ref<DescriptorSetLayout> GetLayout() const override { return this->m_layout; }

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanDescriptorSet>(device);
	}

private:
	VkDevice m_device;
	VkDescriptorSet m_descriptorSet;
	VkDescriptorPool m_pool;

	Ref<DescriptorSetLayout> m_layout;

	Deque<Vector<VkDescriptorBufferInfo>> m_bufferInfos;
	Deque<Vector<VkDescriptorImageInfo>> m_imageInfos;
	Vector<VkWriteDescriptorSet> m_pendingWrites;
};