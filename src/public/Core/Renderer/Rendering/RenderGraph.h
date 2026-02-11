#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/Device.h"
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/Rendering/TransientResourcePool.h"
#include "Core/Renderer/Rendering/GraphNode.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"
#include "Core/Renderer/Rendering/RenderGraphContext.h"

#include <map>

class RenderGraph {
public:
	void Setup(Ref<Device> device, uint32_t nFramesInFlight);

	TextureHandle ImportBackbuffer(Ref<GPUTexture> img, Ref<ImageView> view);
	TextureHandle ImportTexture(Ref<GPUTexture> img, Ref<ImageView> view);

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

		/* Reusing render pass and framebuffer from cache if exists */
		String nodeName(name);
		if (this->m_cachedRenderPasses.count(nodeName)) {
			node.renderPass = this->m_cachedRenderPasses[nodeName];
			
			if (this->m_cachedFramebuffers.count(nodeName)) {
				if (this->m_cachedFramebuffers[nodeName].size() > this->m_nFrameIndex) {
					node.framebuffer = this->m_cachedFramebuffers[nodeName][this->m_nFrameIndex];
				}
			}
		}

		this->m_nodes.push_back(std::move(node));
	}

	void Compile();
	void Execute(Ref<GraphicsContext> context);
	void Reset(uint32_t nFrameIndex);
	void Invalidate();

	TransientResourcePool& GetPool() { return this->m_pool; }
private:
	void CreateRenderPasses();
	void CreateFramebuffers();

	Ref<Device> m_device;
	TransientResourcePool m_pool;
	Vector<GraphNode> m_nodes;
	bool m_bCompiled = false;
	
	uint32_t m_nFramesInFlight = 0;
	uint32_t m_nFrameIndex = 0;

	std::map<String, Ref<RenderPass>> m_cachedRenderPasses;
	std::map<String, Vector<Ref<Framebuffer>>> m_cachedFramebuffers;
};