#pragma once
#include "Core/Renderer/Pipeline.h"
#include "Core/Renderer/Vulkan/VulkanHelpers.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanPipeline : public Pipeline {
public:
	using Ptr = Ref<VulkanPipeline>;

	explicit VulkanPipeline(VkDevice device);
	~VulkanPipeline() override;

	void CreateGraphics(const GraphicsPipelineCreateInfo& createInfo) override;
	void CreateCompute(const ComputePipelineCreateInfo& createInfo) override;
	
	VkPipeline GetVkPipeline() const { this->m_pipeline; }
	VkPipelineLayout GetVkPipelineLayout() const { return this->m_pipelineLayout; }
	VkPipelineBindPoint GetVkBindPoint() const { return this->m_bindPoint; }

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanPipeline>(device);
	}

private:
	VkDevice m_device;
	VkPipeline m_pipeline;
	VkPipelineLayout m_pipelineLayout;
	VkPipelineBindPoint m_bindPoint;

	VkShaderModule CreateShaderModule(const Vector<uint32_t>& shaderCode);
};