#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>kanDevice(

#include "Core/Renderer/DescriptorSetLayout.h"
#include "Core/Renderer/DescriptorPool.h"
#include "Core/Renderer/RenderPass.h"
#include "Core/Renderer/Pipeline.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/ImageView.h"
#include "Core/Renderer/Device.h"

namespace VulkanHelpers {
	/** 
	* Converts EDescriptorType into VkDescriptorType 
	* 
	* @param type Descriptor type
	* 
	* @returns Vulkan descriptor type
	*/
	inline VkDescriptorType
	ConvertDescriptorType(EDescriptorType type) {
		switch (type) {
		case EDescriptorType::SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
		case EDescriptorType::COMBINED_IMAGE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case EDescriptorType::SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case EDescriptorType::STORAGE_IMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case EDescriptorType::UNIFORM_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		case EDescriptorType::STORAGE_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		case EDescriptorType::UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case EDescriptorType::STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case EDescriptorType::UNIFORM_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		case EDescriptorType::STORAGE_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		case EDescriptorType::INPUT_ATTACHMENT: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		default: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		}
	}
	
	/**
	* Converts a single shader stage into Vulkan shader stage
	* 
	* @param stage Shader stage
	* 
	* @returns Vulkan shader stage
	*/
	inline VkShaderStageFlagBits
	ConvertSingleShaderStage(EShaderStage stage) {
		switch (stage) {
			case EShaderStage::VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
			case EShaderStage::FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
			case EShaderStage::GEOMETRY: return VK_SHADER_STAGE_GEOMETRY_BIT;
			case EShaderStage::COMPUTE: return VK_SHADER_STAGE_COMPUTE_BIT;
			case EShaderStage::TESSELATION_CONTROL: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			case EShaderStage::TESSELATION_EVALUATION: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			default: return VK_SHADER_STAGE_VERTEX_BIT;
		}
	}

	/** 
	* Converts EShaderStage into VkShaderStageFlags 
	* 
	* @param EShaderStage Shader stages
	* 
	* @returns Vulkan shader stages
	*/
	inline VkShaderStageFlags
	ConvertShaderStage(EShaderStage stage) {
		VkShaderStageFlags flags = 0;


		if ((stage & EShaderStage::VERTEX) != static_cast<EShaderStage>(0))
			flags |= VK_SHADER_STAGE_VERTEX_BIT;
		if ((stage & EShaderStage::FRAGMENT) != static_cast<EShaderStage>(0))
			flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		if ((stage & EShaderStage::GEOMETRY) != static_cast<EShaderStage>(0))
			flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
		if ((stage & EShaderStage::COMPUTE) != static_cast<EShaderStage>(0))
			flags |= VK_SHADER_STAGE_COMPUTE_BIT;
		if ((stage & EShaderStage::TESSELATION_CONTROL) != static_cast<EShaderStage>(0))
			flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		if ((stage & EShaderStage::TESSELATION_EVALUATION) != static_cast<EShaderStage>(0))
			flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

		/*
			Vulkan doesn't allow to create a descriptor set layout
			without stages, so we create it with all the possible
			shader stages and done
		*/
		if (flags == 0) flags = VK_SHADER_STAGE_ALL;

		return flags;
	}

	/**
	* Converts EPrimitiveTopology to Vulkan primite topology (VkPrimitiveTopology)
	* 
	* @param topology Primitive topology
	* 
	* @returns Vulkan primitive topology
	*/
	inline VkPrimitiveTopology
	ConvertTopology(EPrimitiveTopology topology) {
		switch (topology) {
			case EPrimitiveTopology::POINT_LIST: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			case EPrimitiveTopology::LINE_LIST: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			case EPrimitiveTopology::LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
			case EPrimitiveTopology::TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			case EPrimitiveTopology::TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			case EPrimitiveTopology::TRIANGLE_FAN: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
			case EPrimitiveTopology::PATCH_LIST: return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}

	/**
	* Converts EStencilOp to Vulkan stencil op (VkStencilOp)
	* 
	* @param stencilOp Stencil op
	* 
	* @returns Vulkan stencil op
	*/
	inline VkStencilOp
	ConvertStencilOp(EStencilOp stencilOp) {
		switch (stencilOp) {
			case EStencilOp::KEEP: return VK_STENCIL_OP_KEEP;
			case EStencilOp::ZERO: return VK_STENCIL_OP_ZERO;
			case EStencilOp::REPLACE: return VK_STENCIL_OP_REPLACE;
			case EStencilOp::INCREMENT_AND_CLAMP: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			case EStencilOp::DECREMENT_AND_CLAMP: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			case EStencilOp::INVERT: return VK_STENCIL_OP_INVERT;
			case EStencilOp::INCREMENT_AND_WRAP: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
			case EStencilOp::DECREMENT_AND_WRAP: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
			default: return VK_STENCIL_OP_KEEP;
		}
	}

	/**
	* (EBufferType -> VkBufferUsageFlagBits)
	* 
	* @param bufferType Buffer type
	* 
	* @returns Vulkan buffer usage flag
	*/
	inline VkBufferUsageFlagBits 
	ConvertBufferUsage(EBufferType bufferType) {
		switch (bufferType) {
			case EBufferType::UNIFORM_BUFFER: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			case EBufferType::VERTEX_BUFFER: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			case EBufferType::INDEX_BUFFER: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			case EBufferType::STORAGE_BUFFER:
				return static_cast<VkBufferUsageFlagBits>(
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
					VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
					VK_BUFFER_USAGE_TRANSFER_DST_BIT
					);
			default: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		};
	}

	/**
	* (EImageViewType -> VkImageViewType)
	* 
	* @param viewType View type
	*
	* @returns Vulkan image view type
	*/
	inline VkImageViewType
	ConvertImageViewType(EImageViewType viewType) {
		switch(viewType) {
			case EImageViewType::TYPE_1D: return VK_IMAGE_VIEW_TYPE_1D;
			case EImageViewType::TYPE_2D: return VK_IMAGE_VIEW_TYPE_2D;
			case EImageViewType::TYPE_CUBE: return VK_IMAGE_VIEW_TYPE_CUBE;
			case EImageViewType::TYPE_1D_ARRAY: return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			case EImageViewType::TYPE_2D_ARRAY: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			case EImageViewType::TYPE_CUBE_ARRAY: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			default: return VK_IMAGE_VIEW_TYPE_2D;
		}
	}

	/**
	* (EImageAspect -> VkImageAspectFlags)
	* 
	* @param aspectMask Image aspect
	* 
	* @returns Vulkan image aspect
	*/
	inline VkImageAspectFlags
	ConvertImageAspect(EImageAspect aspectMask) {
		VkImageAspectFlags flags = 0;

		if ((aspectMask & EImageAspect::COLOR) != static_cast<EImageAspect>(0)) {
			flags |= VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if ((aspectMask & EImageAspect::DEPTH) != static_cast<EImageAspect>(0)) {
			flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		if ((aspectMask & EImageAspect::STENCIL) != static_cast<EImageAspect>(0)) {
			flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		return flags;
	}

	/**
	* (EAccess -> VkAccessFlags)
	* 
	* @param access Access flags
	* 
	* @returns Vulkan access flags
	*/
	inline VkAccessFlags
	ConvertAccess(EAccess access) {
		VkAccessFlags vkAccess = VK_ACCESS_NONE;

		if ((access & EAccess::INDIRECT_COMMAND_READ) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		}

		if ((access & EAccess::INDEX_READ) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_INDEX_READ_BIT;
		}

		if ((access & EAccess::VERTEX_ATTRIBUTE_READ) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		}

		if ((access & EAccess::UNIFORM_READ) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_UNIFORM_READ_BIT;
		}

		if ((access & EAccess::INPUT_ATTACHMENT_READ) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		}

		if ((access & EAccess::SHADER_READ) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_SHADER_READ_BIT;
		}
		
		if ((access & EAccess::SHADER_WRITE) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_SHADER_WRITE_BIT;
		}

		if ((access & EAccess::COLOR_ATTACHMENT_READ) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		}

		if ((access & EAccess::COLOR_ATTACHMENT_WRITE) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}

		if ((access & EAccess::DEPTH_STENCIL_READ) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		}

		if ((access & EAccess::DEPTH_STENCIL_WRITE) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}

		if ((access & EAccess::TRANSFER_READ) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_TRANSFER_READ_BIT;
		}

		if ((access & EAccess::TRANSFER_WRITE) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_TRANSFER_WRITE_BIT;
		}

		if ((access & EAccess::HOST_READ) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_HOST_READ_BIT;
		}

		if ((access & EAccess::HOST_WRITE) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_HOST_WRITE_BIT;
		}

		if ((access & EAccess::MEMORY_READ) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_MEMORY_READ_BIT;
		}

		if ((access & EAccess::MEMORY_WRITE) != EAccess::NONE) {
			vkAccess |= VK_ACCESS_MEMORY_WRITE_BIT;
		}

		return vkAccess;
	}

	/**
	* (EPipelineStage -> VkPipelineStageFlags)
	* 
	* @param stage Pipeline stage
	* 
	* @returns Vulkan pipeline stage
	*/
	inline VkPipelineStageFlags
	ConvertPipelineStage(EPipelineStage stage) {
		VkPipelineStageFlags vkStage = 0;

		if ((stage & EPipelineStage::TOP_OF_PIPE) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}

		if ((stage & EPipelineStage::DRAW_INDIRECT) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
		}

		if ((stage & EPipelineStage::VERTEX_INPUT) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		}

		if ((stage & EPipelineStage::VERTEX_SHADER) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		}

		if ((stage & EPipelineStage::TESSELLATION_CONTROL) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
		}

		if ((stage & EPipelineStage::TESSELLATION_EVAL) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
		}

		if ((stage & EPipelineStage::GEOMETRY) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
		}

		if ((stage & EPipelineStage::FRAGMENT) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		if ((stage & EPipelineStage::EARLY_FRAGMENT_TESTS) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}

		if ((stage & EPipelineStage::LATE_FRAGMENT_TESTS) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}

		if ((stage & EPipelineStage::COLOR_ATTACHMENT_OUTPUT) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}

		if ((stage & EPipelineStage::COMPUTE_SHADER) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}

		if ((stage & EPipelineStage::TRANSFER) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		}

		if ((stage & EPipelineStage::BOTTOM_OF_PIPE) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}

		if ((stage & EPipelineStage::HOST) != static_cast<EPipelineStage>(0)) {
			vkStage |= VK_PIPELINE_STAGE_HOST_BIT;
		}

		return vkStage;
	}

	/**
	* (ESharingMode -> VkSharingMode)
	* 
	* @param sharingMode Sharing mode
	* 
	* @returns Vulkan sharing mode
	*/
	inline VkSharingMode
	ConvertSharingMode(ESharingMode sharingMode) {
		switch (sharingMode) {
			case ESharingMode::CONCURRENT: return VK_SHARING_MODE_CONCURRENT;
			case ESharingMode::EXCLUSIVE: return VK_SHARING_MODE_EXCLUSIVE;
			default: return VK_SHARING_MODE_EXCLUSIVE;
		}
	}

	inline VkBlendFactor
	ConvertBlendFactor(EBlendFactor blendFactor) {
		switch(blendFactor) {
			case EBlendFactor::ZERO: return VK_BLEND_FACTOR_ZERO;
			case EBlendFactor::ONE: return VK_BLEND_FACTOR_ONE;
			case EBlendFactor::SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
			case EBlendFactor::ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			case EBlendFactor::DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
			case EBlendFactor::ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			case EBlendFactor::SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
			case EBlendFactor::ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			case EBlendFactor::DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
			case EBlendFactor::ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			case EBlendFactor::CONSTANT_COLOR: return VK_BLEND_FACTOR_CONSTANT_COLOR;
			case EBlendFactor::ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
			default: return VK_BLEND_FACTOR_ZERO;
		}
	}

	inline VkAttachmentLoadOp 
	ConvertLoadOp(EAttachmentLoadOp loadOp) {
		switch (loadOp) {
			case EAttachmentLoadOp::LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
			case EAttachmentLoadOp::CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
			case EAttachmentLoadOp::DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			default: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		}
	}

	inline VkAttachmentStoreOp
	ConvertStoreOp(EAttachmentStoreOp storeOp) {
		switch(storeOp) {
			case EAttachmentStoreOp::STORE: return VK_ATTACHMENT_STORE_OP_STORE;
			case EAttachmentStoreOp::DONT_CARE: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
			default: return VK_ATTACHMENT_STORE_OP_STORE;
		}
	}

	inline VkImageLayout
	ConvertImageLayout(EImageLayout layout) {
		switch (layout) {
			case EImageLayout::UNDEFINED: return VK_IMAGE_LAYOUT_UNDEFINED;
			case EImageLayout::GENERAL: return VK_IMAGE_LAYOUT_GENERAL;
			case EImageLayout::COLOR_ATTACHMENT: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			case EImageLayout::DEPTH_STENCIL_ATTACHMENT: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			case EImageLayout::DEPTH_STENCIL_READ_ONLY: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			case EImageLayout::SHADER_READ_ONLY: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case EImageLayout::TRANSFER_SRC: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			case EImageLayout::TRANSFER_DST: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			case EImageLayout::PRESENT_SRC: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			default: return VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}

	inline VkFormat 
	ConvertFormat(GPUFormat format) {
		switch (format) {
			case GPUFormat::RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
			case GPUFormat::BGRA8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
			case GPUFormat::RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
			case GPUFormat::RGB32_FLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
			case GPUFormat::RG32_FLOAT: return VK_FORMAT_R32G32_SFLOAT;
			case GPUFormat::RGBA16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
			case GPUFormat::RGBA32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
			case GPUFormat::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
			case GPUFormat::D32_FLOAT: return VK_FORMAT_D32_SFLOAT;
			case GPUFormat::D32_FLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;
			case GPUFormat::R8_UNORM: return VK_FORMAT_R8_UNORM;
			default: return VK_FORMAT_R8G8B8A8_UNORM;
		}
	}

	inline GPUFormat
	RevertFormat(VkFormat format) {
		switch (format) {
			case VK_FORMAT_R8G8B8A8_UNORM: return GPUFormat::RGBA8_UNORM;
			case VK_FORMAT_B8G8R8A8_UNORM: return GPUFormat::BGRA8_UNORM;
			case VK_FORMAT_R8G8B8A8_SRGB: return GPUFormat::RGBA8_SRGB;
			case VK_FORMAT_R32G32B32_SFLOAT: return GPUFormat::RGB32_FLOAT;
			case VK_FORMAT_R32G32_SFLOAT: return GPUFormat::RG32_FLOAT;
			case VK_FORMAT_R16G16B16A16_SFLOAT: return GPUFormat::RGBA16_FLOAT;
			case VK_FORMAT_R32G32B32A32_SFLOAT: return GPUFormat::RGBA32_FLOAT;
			case VK_FORMAT_D24_UNORM_S8_UINT: return GPUFormat::D24_UNORM_S8_UINT;
			case VK_FORMAT_D32_SFLOAT: return GPUFormat::D32_FLOAT;
			case VK_FORMAT_D32_SFLOAT_S8_UINT: return GPUFormat::D32_FLOAT_S8_UINT;
			case VK_FORMAT_R8_UNORM: return GPUFormat::R8_UNORM;
			default: return GPUFormat::RGBA8_UNORM;
		}
	}

	inline VkCullModeFlags
	ConvertCullMode(ECullMode cullMode) {
		switch (cullMode) {
			case ECullMode::NONE: return VK_CULL_MODE_NONE;
			case ECullMode::FRONT: return VK_CULL_MODE_FRONT_BIT;
			case ECullMode::BACK: return VK_CULL_MODE_BACK_BIT;
			case ECullMode::FRONT_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
			default: return VK_CULL_MODE_NONE;
		}
	}
	
	inline VkPolygonMode
	ConvertPolygonMode(EPolygonMode polygonMode) {
		switch (polygonMode) {
			case EPolygonMode::FILL: return VK_POLYGON_MODE_FILL;
			case EPolygonMode::LINE: return VK_POLYGON_MODE_LINE;
			case EPolygonMode::POINT: return VK_POLYGON_MODE_POINT;
			default: return VK_POLYGON_MODE_FILL;
		}
	}

	inline VkFrontFace
	ConvertFrontFace(EFrontFace frontFace) {
		switch (frontFace) {
			case EFrontFace::CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;
			case EFrontFace::COUNTER_CLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
			default: return VK_FRONT_FACE_CLOCKWISE;
		}
	}
	
	inline VkCompareOp
	ConvertCompareOp(ECompareOp compareOp) {
		switch (compareOp) {
			case ECompareOp::ALWAYS: return VK_COMPARE_OP_ALWAYS;
			case ECompareOp::EQUAL: return VK_COMPARE_OP_EQUAL;
			case ECompareOp::GREATER: return VK_COMPARE_OP_GREATER;
			case ECompareOp::GREATER_OR_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case ECompareOp::LESS: return VK_COMPARE_OP_LESS;
			case ECompareOp::LESS_OR_EQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
			case ECompareOp::NEVER: return VK_COMPARE_OP_NEVER;
			case ECompareOp::NOT_EQUAL: return VK_COMPARE_OP_NOT_EQUAL;
			default: return VK_COMPARE_OP_LESS;
		}
	}

	inline VkBlendOp
	ConvertBlendOp(EBlendOp blendOp) {
		switch(blendOp) {
			case EBlendOp::ADD: return VK_BLEND_OP_ADD;
			case EBlendOp::MAX: return VK_BLEND_OP_MAX;
			case EBlendOp::MIN: return VK_BLEND_OP_MIN;
			case EBlendOp::REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
			case EBlendOp::SUBTRACT: return VK_BLEND_OP_SUBTRACT;
			default: return VK_BLEND_OP_ADD;
		}
	}

	inline VkIndexType
	ConvertIndexType(EIndexType indexType) {
		switch (indexType) {
			case EIndexType::UINT8: return VK_INDEX_TYPE_UINT8;
			case EIndexType::UINT16: return VK_INDEX_TYPE_UINT16;
			case EIndexType::UINT32: return VK_INDEX_TYPE_UINT32;
			default: return VK_INDEX_TYPE_UINT16;
		}
	}
}