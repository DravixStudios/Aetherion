#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/Device.h"
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/Rendering/TransientResourcePool.h"
#include "Core/Renderer/Rendering/GraphNode.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"

class RenderGraph {
public:
	void Setup(Ref<Device> device);

	TextureHandle ImportBackbuffer(Ref<GPUTexture> img, Ref<ImageView> view);

	/**
	* Adds a node to the render graph
	* 
	* @param name Node name
	* @param setup Setup function
	* @param execute Execute function
	*/
	template<typename Setup, typename Execute>
	void AddNode(const char* name, Setup&& setup, Execute&& execute) {
		GraphNode node;
		node.name = name;
		
		RenderGraphBuilder builder;
		builder.m_node = &node;
		builder.m_pool = &this->m_pool;
		setup(builder);

		node.execute = std::forward<Execute>(execute);
		this->m_nodes.push_back(std::move(node));
	}

	void Compile();
	void Execute(Ref<GraphicsContext> context);
	void Reset();

	TransientResourcePool& GetPool() { return this->m_pool; }
private:
	void CreateRenderPasses();
	void CreateFramebuffers();

	Ref<Device> m_device;
	TransientResourcePool m_pool;
	Vector<GraphNode> m_nodes;
};