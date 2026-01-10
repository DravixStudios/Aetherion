#pragma once
#include <iostream>
#include "Core/Containers.h"
#include "Core/Renderer/Shader.h"
#include "Core/Renderer/GPUFormat.h"

#include "Core/Renderer/DescriptorSetLayout.h"
#include "Core/Renderer/RenderPass.h"
#include "Core/Renderer/PipelineLayout.h"

enum class EPipelineType {
	GRAPHICS,
	COMPUTE
};

enum class EPrimitiveTopology {
	POINT_LIST,
	LINE_LIST,
	LINE_STRIP,
	TRIANGLE_LIST,
	TRIANGLE_STRIP,
	TRIANGLE_FAN,
	PATCH_LIST
};

enum class ECullMode {
	NONE,
	FRONT,
	BACK,
	FRONT_BACK
};

enum class EPolygonMode {
	FILL,
	LINE,
	POINT
};


enum class EFrontFace {
	CLOCKWISE,
	COUNTER_CLOCKWISE
};

enum class ECompareOp {
	NEVER,
	LESS,
	EQUAL,
	LESS_OR_EQUAL,
	GREATER,
	NOT_EQUAL,
	GREATER_OR_EQUAL,
	ALWAYS
};

enum class EBlendFactor {
	ZERO,
	ONE,
	SRC_COLOR,
	ONE_MINUS_SRC_COLOR,
	DST_COLOR,
	ONE_MINUS_DST_COLOR,
	SRC_ALPHA,
	ONE_MINUS_SRC_ALPHA,
	DST_ALPHA,
	ONE_MINUS_DST_ALPHA,
	CONSTANT_COLOR,
	ONE_MINUS_CONSTANT_COLOR
};

enum class EBlendOp {
	ADD,
	SUBTRACT,
	REVERSE_SUBTRACT,
	MIN,
	MAX
};

enum class EStencilOp {
	KEEP,
	ZERO,
	REPLACE,
	INCREMENT_AND_CLAMP,
	DECREMENT_AND_CLAMP,
	INVERT,
	INCREMENT_AND_WRAP,
	DECREMENT_AND_WRAP
};


struct VertexInputBinding {
	uint32_t nBinding;
	uint32_t nStride;
	bool bPerInstance; // false: per vertex, true: per instance
}; 

struct VertexInputAttribute {
	uint32_t nLocation;
	uint32_t nBinding;
	GPUFormat format;
	uint32_t nOffset;
};

struct RasterizationState {
	ECullMode cullMode = ECullMode::BACK;
	EFrontFace frontFace = EFrontFace::COUNTER_CLOCKWISE;
	EPolygonMode polygonMode = EPolygonMode::FILL;
	float lineWidth = 1.f;
	bool bDepthClampEnable = false;
	bool bRasterizerDiscardEnable = false;
	float depthBiasConstantFactor = 0.f;
	float depthBiasClamp = 0.f;
	float depthBiasSlopeFactor = 0.f;
	bool bDepthBiasEnable = false;
};

struct DepthStencilState {
	bool bDepthTestEnable = true;
	bool bDepthWriteEnable = true;
	ECompareOp depthCompareOp = ECompareOp::LESS;
	bool bDepthBoundsTestEnable = false;
	float minDepthBounds = 0.f;
	float maxDepthBounds = 1.f;

	bool bStencilTestEnable = false;
	EStencilOp stencilFailOp = EStencilOp::KEEP;
	EStencilOp stencilPassOp = EStencilOp::KEEP;
	EStencilOp stencilDepthFailOp = EStencilOp::KEEP;
	ECompareOp stencilCompareOp = ECompareOp::ALWAYS;
	uint32_t stencilCompareMask = 0xFF;
	uint32_t stencilWriteMask = 0xFF;
	uint32_t stencilReference = 0;
};

struct ColorBlendAttachment {
	bool bBlendEnable = false;
	EBlendFactor srcColorBlendFactor = EBlendFactor::ONE;
	EBlendFactor dstColorBlendFactor = EBlendFactor::ZERO;
	EBlendOp colorBlendOp = EBlendOp::ADD;
	EBlendFactor srcAlphaBlendFactor = EBlendFactor::ONE;
	EBlendFactor dstAlphaBlendFactor = EBlendFactor::ZERO;
	EBlendOp alphaBlendOp = EBlendOp::ADD;
	bool bWriteR = false;
	bool bWriteG = false;
	bool bWriteB = false;
	bool bWriteA = false;
};

struct ColorBlendState {
	bool bLogicOpEnable = false;
	Vector<ColorBlendAttachment> attachments;
	float blendConstants[4] = { 0.f, 0.f, 0.f, 0.f };
};

struct MultisampleState {
	uint32_t nSampleCount = 1;
	bool bSampleShadingEnable = false;
	float minSampleShading = 1.f;
	bool bAlphaToCoverageEnable = false;
	bool bAlphaToOneEnable = false;
};

struct GraphicsPipelineCreateInfo {
	/* Shaders */
	Vector<Ref<Shader>> shaders;

	/* Vertex input */
	Vector<VertexInputBinding> vertexBindings;
	Vector<VertexInputAttribute> vertexAttributes;

	/* Input assembly */
	EPrimitiveTopology topology = EPrimitiveTopology::TRIANGLE_LIST;
	bool bPrimitiveRestartEnable = false;

	/* Rasterization */
	RasterizationState rasterizationState;

	/* Depth stencil */
	DepthStencilState depthStencilState;

	/* Color blend */
	ColorBlendState colorBlendState;

	/* Multisampling */
	MultisampleState multisampleState;

	/* Pipeline layout */
	Ref<PipelineLayout> pipelineLayout;

	/* Render pass (only for Vulkan, nullptr in D3D12 and others) */
	Ref<RenderPass> renderPass { std::shared_ptr<RenderPass>(nullptr) };
	uint32_t nSubpass;
};

struct ComputePipelineCreateInfo {
	Ref<Shader> shader;
	Vector<Ref<DescriptorSetLayout>> descriptorSetLayouts;
	Vector<PushConstantRange> pushConstantRanges;
};

class Pipeline {
public:
	static constexpr const char* CLASS_NAME = "Pipeline";
	using Ptr = Ref<Pipeline>;

	virtual ~Pipeline() = default;
	virtual void CreateGraphics(const GraphicsPipelineCreateInfo& createInfo) = 0;
	virtual void CreateCompute(const ComputePipelineCreateInfo& createInfo) = 0;

	EPipelineType GetType() const { return this->m_type; }
protected:
	EPipelineType m_type;
};