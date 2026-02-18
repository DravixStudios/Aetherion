#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/GraphicsContext.h"
#include "Core/Renderer/Pipeline.h"
#include "Core/Renderer/RenderPass.h"
#include "Core/Renderer/Semaphore.h"
#include "Core/Renderer/Fence.h"
#include "Core/Renderer/Swapchain.h"

#include <GLFW/glfw3.h>

struct ImGuiImplCreateInfo {
	GLFWwindow* pWindow = nullptr;
	Ref<GraphicsContext> context;
	Ref<RenderPass> renderPass;
	Ref<DescriptorPool> descriptorPool;
};

class ImGuiImpl {
public:
	static constexpr const char* CLASS_NAME = "ImGuiImpl";

	using Ptr = Ref<ImGuiImpl>;
	
	virtual ~ImGuiImpl() = default;
	
	virtual void Create(const ImGuiImplCreateInfo& createInfo) = 0;

	virtual void NewFrame() = 0;
	virtual void Render() = 0;
};