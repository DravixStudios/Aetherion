#pragma once
#include "Core/Renderer/ImGuiImpl.h"

#include "Core/Renderer/Vulkan/VulkanGraphicsContext.h"
#include "Core/Renderer/Vulkan/VulkanDevice.h";

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_glfw.h>

class VulkanImGuiImpl : public ImGuiImpl {
public:
	using Ptr = Ref<VulkanImGuiImpl>;

	explicit VulkanImGuiImpl(Ref<VulkanDevice> device);

	void Create(const ImGuiImplCreateInfo& createInfo) override;
	void NewFrame() override;
	void Render(Ref<GraphicsContext> context) override;

	Ref<DescriptorSet> AddTexture(
		Ref<Sampler> sampler,
		Ref<ImageView> imageView,
		EImageLayout imageLayout
	) override;

	void RemoveTexture(Ref<DescriptorSet> set) override;

	void Image(Ref<DescriptorSet> descriptorSet, ImVec2 imageSize) override;
	bool ImageButton(Ref<DescriptorSet> descriptorSet, const String& label, ImVec2 size) override;

	static Ptr
	CreateShared(Ref<VulkanDevice> device) {
		return CreateRef<VulkanImGuiImpl>(device);
	}

private:
	Ref<VulkanDevice> m_device;
	Ref<DescriptorPool> m_pool;

	GLFWwindow* m_pWindow;
};
