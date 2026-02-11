#include "Core/Renderer/Rendering/RenderGraph.h"

/**
* Setups the render graph
* @param device Logical device
*/
void 
RenderGraph::Setup(Ref<Device> device, uint32_t nFramesInFlight){
    this->m_device = device;
    this->m_pool.Init(device);
    this->m_nFramesInFlight = nFramesInFlight;
}

/**
* Imports a back buffer
*
* @param image Back buffer image
* @param view Back buffer image view
*
* @returns Texture handle
*/
TextureHandle 
RenderGraph::ImportBackbuffer(Ref<GPUTexture> image, Ref<ImageView> view) {
    return this->m_pool.ImportTexture(image, view);
}

/**
* Imports a texture
*
* @param image Image to import
* @param view Image view to import
*
* @returns Texture handle
*/
TextureHandle
RenderGraph::ImportTexture(Ref<GPUTexture> image, Ref<ImageView> view) {
    return this->m_pool.ImportTexture(image, view);
}

/**
* Compiles the render graph
*/
void
RenderGraph::Compile() {
    if (!this->m_bCompiled) {
        this->CreateRenderPasses();
        this->CreateFramebuffers();
        this->m_bCompiled = true;
    }
}

/**
* Creates the render passes
*/
void
RenderGraph::CreateRenderPasses() {
    for(GraphNode& node : this->m_nodes) {
        if (node.bIsComputeOnly) {
             continue;
        }

        // Reuse cached render pass if available
        if (node.renderPass) {
             continue;
        }

        RenderPassCreateInfo rpInfo = { };
        
        /* Color attachments */
        for(uint32_t i = 0; i < node.colorOutputs.size(); ++i) {
            TextureHandle& color = node.colorOutputs[i];
            Ref<ImageView> view = this->m_pool.GetImageView(color);

            AttachmentDescription attachment = { };
            attachment.format = view->GetFormat();
            attachment.sampleCount = ESampleCount::SAMPLE_1;
            attachment.initialLayout = EImageLayout::UNDEFINED;
            attachment.finalLayout = node.colorFinalLayouts[i];
            attachment.loadOp = node.colorLoadOps[i];
            attachment.storeOp = EAttachmentStoreOp::STORE;
            attachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
            attachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;

            rpInfo.attachments.push_back(attachment);
        }

        /* Depth attachment */
        if(node.bHasDepth) {
            Ref<ImageView> depthView = this->m_pool.GetImageView(node.depthOutput);

            AttachmentDescription depthAttachment = { };
            depthAttachment.format = depthView->GetFormat();
            depthAttachment.sampleCount = ESampleCount::SAMPLE_1;
            depthAttachment.initialLayout = EImageLayout::UNDEFINED;
            depthAttachment.finalLayout = node.depthFinalLayout;
            depthAttachment.loadOp = node.depthLoadOp;
            depthAttachment.storeOp = EAttachmentStoreOp::STORE;
            depthAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
            depthAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;

            rpInfo.attachments.push_back(depthAttachment);
        }

        /* Subpass */
        SubpassDescription subpass = { };
        for(uint32_t i = 0; i < node.colorOutputs.size(); ++i) {
            subpass.colorAttachments.push_back({ i, EImageLayout::COLOR_ATTACHMENT });
        }

        if(node.bHasDepth) {
            subpass.bHasDepthStencil = true;
            subpass.depthStencilAttachment = {
                static_cast<uint32_t>(node.colorOutputs.size()),
                EImageLayout::DEPTH_STENCIL_ATTACHMENT
            };
        }

        rpInfo.subpasses.push_back(subpass);

        node.renderPass = this->m_device->CreateRenderPass(rpInfo);

        this->m_cachedRenderPasses[String(node.name)] = node.renderPass;
    }
}

/**
* Creates the framebuffers
*/
void
RenderGraph::CreateFramebuffers() {
    for(GraphNode& node : this->m_nodes) {
        if (node.bIsComputeOnly) {
            continue;
        }

        FramebufferCreateInfo fbInfo = { };
        fbInfo.renderPass = node.renderPass;

        for(TextureHandle& color : node.colorOutputs) {
            fbInfo.attachments.push_back(this->m_pool.GetImageView(color));
        }

        if(node.bHasDepth) {
            fbInfo.attachments.push_back(this->m_pool.GetImageView(node.depthOutput));
        }

        fbInfo.nWidth = node.nWidth;
        fbInfo.nHeight = node.nHeight;

        node.framebuffer = this->m_device->CreateFramebuffer(fbInfo);

        String nodeName(node.name);
        if (this->m_cachedFramebuffers[nodeName].size() < this->m_nFramesInFlight) {
            this->m_cachedFramebuffers[nodeName].resize(this->m_nFramesInFlight);
        }

        this->m_cachedFramebuffers[nodeName][this->m_nFrameIndex] = node.framebuffer;
    }
}

/**
* Executes the render graph
* 
* @param context Graphics context
*/
void 
RenderGraph::Execute(Ref<GraphicsContext> context) {
    RenderGraphContext graphCtx;
    graphCtx.m_pool = &this->m_pool;

    for(GraphNode& node : this->m_nodes) {
        if (node.bIsComputeOnly) {
            node.execute(context, graphCtx);
            continue;
        }

        /* Synchronization */
        context->GlobalBarrier();

        /* Prepare the render pass */
        RenderPassBeginInfo beginInfo = { };
        beginInfo.renderPass = node.renderPass;
        beginInfo.framebuffer = node.framebuffer;
        beginInfo.renderArea = { { 0, 0 }, { node.nWidth, node.nHeight } };

        for(uint32_t i = 0; i < node.colorOutputs.size(); ++i) {
            beginInfo.clearValues.push_back(ClearValue { ClearColor {0.f, 0.f, 0.f, 1.f} });
        }

        if(node.bHasDepth) {
            beginInfo.clearValues.push_back(ClearValue { ClearDepthStencil { 1.f, 0 } });
        }

        /* Begin the render pass */
        context->BeginRenderPass(beginInfo);

        /* Execute the graph node */
        node.execute(context, graphCtx);

        /* End the render pass */
        context->EndRenderPass();
    }

    this->m_pool.EndFrame();
}

/**
* Resets the render graph
*/
void
RenderGraph::Reset(uint32_t nFrameIndex) {
    this->m_nodes.clear();
    this->m_pool.BeginFrame();
    this->m_nFrameIndex = nFrameIndex;
    this->m_bCompiled = false;
}

/**
* Invalidates the render graph
*/
void
RenderGraph::Invalidate() {
    this->m_nodes.clear();
    this->m_bCompiled = false;
}
