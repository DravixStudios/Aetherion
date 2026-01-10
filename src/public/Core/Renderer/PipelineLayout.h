#pragma once
#include <iostream>
#include "Core/Renderer/Shader.h"

struct PushConstantRange {
	EShaderStage stage;
	uint32_t nOffset;
	uint32_t nSize;
};

struct PipelineLayoutCreateInfo {
	Vector<Ref<DescriptorSetLayout>> setLayouts;
	Vector<PushConstantRange> pushConstantRanges;
};

class PipelineLayout {
public:
	static constexpr const char* CLASS_NAME = "PipelineLayout";
	using Ptr = Ref<PipelineLayout>;

	virtual ~PipelineLayout() = default;

	virtual void Create(const PipelineLayoutCreateInfo& createInfo) = 0;

};