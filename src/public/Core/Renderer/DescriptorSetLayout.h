#pragma once
#include <cstdint>
#include <memory>

#include "Core/Containers.h"

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

enum class EShaderStage {
	VERTEX = 1 << 0,
	FRAGMENT = 1 << 1,
	COMPUTE = 1 << 2,
	GEOMETRY = 1 << 3,
	TESSELATION_CONTROL = 1 << 4,
	TESSELATION_EVALUATION = 1 << 5,
	GRAPHICS_ALL = VERTEX | FRAGMENT | GEOMETRY | TESSELATION_CONTROL | TESSELATION_EVALUATION,
	ALL = 0x7FFFFFFF
};

inline EShaderStage operator|(EShaderStage a, EShaderStage b) {
	return static_cast<EShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline EShaderStage operator&(EShaderStage a, EShaderStage b) {
	return static_cast<EShaderStage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

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