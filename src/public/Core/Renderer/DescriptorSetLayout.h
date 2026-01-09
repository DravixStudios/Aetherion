#pragma once
#include <cstdint>
#include <memory>

#include "Core/Containers.h"
#include "Core/Renderer/Shader.h"

enum class EDescriptorType {
	SAMPLER,
	COMBINED_IMAGE_SAMPLER,
	SAMPLED_IMAGE,
	STORAGE_IMAGE,
	UNIFORM_TEXEL_BUFFER,
	STORAGE_TEXEL_BUFFER,
	UNIFORM_BUFFER,
	STORAGE_BUFFER,
	UNIFORM_BUFFER_DYNAMIC,
	STORAGE_BUFFER_DYNAMIC,
	INPUT_ATTACHMENT
};

struct DescriptorSetLayoutBinding {
	uint32_t nBinding;
	EDescriptorType descriptorType;
	uint32_t nDescriptorCount;
	EShaderStage stageFlags;
	bool bUpdateAfterBind = false; // For bindless
};

struct DescriptorSetLayoutCreateInfo {
	Vector<DescriptorSetLayoutBinding> bindings;
	bool bUpdateAfterBind = false; // If the whole set supports update after bind
};

class DescriptorSetLayout {
public:
	using Ptr = Ref<DescriptorSetLayout>;

	static constexpr const char* CLASS_NAME = "DescriptorSetLayout";

	DescriptorSetLayout() = default;
	virtual ~DescriptorSetLayout() = default;

	virtual void Create(const DescriptorSetLayoutCreateInfo& createInfo) { this->m_createInfo = createInfo; }

	const DescriptorSetLayoutCreateInfo& GetCreateInfo() const { return this->m_createInfo; }
protected:
	DescriptorSetLayoutCreateInfo m_createInfo;
};