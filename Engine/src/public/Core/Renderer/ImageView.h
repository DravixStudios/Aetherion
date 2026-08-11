#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/GPUFormat.h"

enum class EImageViewType {
	TYPE_1D,
	TYPE_2D,
	TYPE_3D,
	TYPE_CUBE,
	TYPE_1D_ARRAY,
	TYPE_2D_ARRAY,
	TYPE_CUBE_ARRAY
};

enum class EImageAspect : uint32_t {
	COLOR = 1,
	DEPTH = 1 << 1,
	STENCIL = 1 << 2,
	DEPTH_STENCIL = DEPTH | STENCIL
};

inline EImageAspect 
operator|(EImageAspect a, EImageAspect b) {
	return static_cast<EImageAspect>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline EImageAspect 
operator&(EImageAspect a, EImageAspect b) {
	return static_cast<EImageAspect>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

struct ComponentMapping {
	enum class ESwizzle {
		IDENTITY,
		ZERO,
		ONE,
		R,
		G,
		B,
		A
	};

	ESwizzle r = ESwizzle::IDENTITY;
	ESwizzle g = ESwizzle::IDENTITY;
	ESwizzle b = ESwizzle::IDENTITY;
	ESwizzle a = ESwizzle::IDENTITY;
};

struct ImageSubresourceRange {
	EImageAspect aspectMask = EImageAspect::COLOR;
	uint32_t nBaseMipLevel = 0;
	uint32_t nLevelCount = 1;
	uint32_t nBaseArrayLayer = 0;
	uint32_t nLayerCount = 1;
};

struct ImageViewCreateInfo {
	Ref<GPUTexture> image;
	EImageViewType viewType = EImageViewType::TYPE_2D;
	GPUFormat format = GPUFormat::UNDEFINED;
	ComponentMapping components = {};
	ImageSubresourceRange subresourceRange = {};
};

class ImageView {
public:
	using Ptr = Ref<ImageView>;

	static constexpr const char* CLASS_NAME = "ImageView";

	virtual ~ImageView() = default;

	/**
	* Creates a image view
	* 
	* @param createInfo Image view create info
	*/
	virtual void Create(const ImageViewCreateInfo& createInfo) = 0;

	/**
	* Gets the subyacent image
	*
	* @returns The image
	*/
	virtual Ref<GPUTexture> GetImage() const = 0;

	/**
	* Gets the view type
	* 
	* @returns The view type
	*/
	virtual EImageViewType GetViewType() const = 0;

	/**
	* Gets the view format
	* 
	* @returns The view format
	*/
	virtual GPUFormat GetFormat() const = 0;

	virtual void Reset() = 0;
};