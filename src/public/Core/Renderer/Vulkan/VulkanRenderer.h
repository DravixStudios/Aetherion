#pragma once
#include "Core/Renderer/Renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanRenderer : public Renderer {
public:
	static constexpr const char* CLASS_NAME = "VulkanRenderer";

	explicit VulkanRenderer();
	~VulkanRenderer() override;

	void Create() override;

private:
	bool m_bEnableValidationLayers;

	bool CheckValidationLayersSupport();

	VkInstance m_instance;
};