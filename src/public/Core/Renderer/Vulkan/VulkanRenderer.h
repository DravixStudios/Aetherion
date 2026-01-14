#pragma once
#include "Core/Renderer/Renderer.h"

class VulkanRenderer : public Renderer {
public:
	explicit VulkanRenderer();
	~VulkanRenderer() override;

	void Create() override;
};