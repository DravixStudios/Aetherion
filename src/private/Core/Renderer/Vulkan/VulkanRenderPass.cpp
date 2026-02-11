#include "Core/Renderer/Vulkan/VulkanRenderPass.h"

VulkanRenderPass::VulkanRenderPass(VkDevice device) 
	: m_device(device), m_renderPass(VK_NULL_HANDLE) {}

VulkanRenderPass::~VulkanRenderPass() {
	if (this->m_renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(this->m_device, this->m_renderPass, nullptr);
	}
}

void 
VulkanRenderPass::Create(const RenderPassCreateInfo& createInfo) {
	RenderPass::Create(createInfo);

	/* (AttachmentDescription -> VkAttachmentDescription) */
	Vector<VkAttachmentDescription2> attachments;
	for (const AttachmentDescription& attachment : createInfo.attachments) {
		VkAttachmentDescription2 vkAttachment = { };
		vkAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
		vkAttachment.format = VulkanHelpers::ConvertFormat(attachment.format);
		vkAttachment.initialLayout = VulkanHelpers::ConvertImageLayout(attachment.initialLayout);
		vkAttachment.finalLayout = VulkanHelpers::ConvertImageLayout(attachment.finalLayout);
		vkAttachment.stencilLoadOp = VulkanHelpers::ConvertLoadOp(attachment.stencilLoadOp);
		vkAttachment.stencilStoreOp = VulkanHelpers::ConvertStoreOp(attachment.stencilStoreOp); 
		vkAttachment.loadOp = VulkanHelpers::ConvertLoadOp(attachment.loadOp);
		vkAttachment.storeOp = VulkanHelpers::ConvertStoreOp(attachment.storeOp);
		vkAttachment.samples = static_cast<VkSampleCountFlagBits>(attachment.nSampleCount);

		attachments.push_back(vkAttachment);
	}

	/* Convert subpasses */
	Vector<VkSubpassDescription2> subpasses;
	Vector<Vector<VkAttachmentReference2>> colorRefs;
	Vector<Vector<VkAttachmentReference2>> resolveRefs;
	Vector<VkAttachmentReference2> depthRefs;
	Vector<VkAttachmentReference2> depthResolveRefs;
	Vector<VkSubpassDescriptionDepthStencilResolve> depthStencilResolves;

	colorRefs.resize(createInfo.subpasses.size());
	resolveRefs.resize(createInfo.subpasses.size());
	depthRefs.resize(createInfo.subpasses.size());
	depthResolveRefs.resize(createInfo.subpasses.size());

	for(uint32_t i = 0; i < createInfo.subpasses.size(); ++i) {
		const SubpassDescription& subpass = createInfo.subpasses[i];

		/* Color attachments */
		for (const AttachmentReference& colorRef : subpass.colorAttachments) {
			VkAttachmentReference2 vkRef = { };
			vkRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
			vkRef.attachment = colorRef.nAttachment;
			vkRef.layout = VulkanHelpers::ConvertImageLayout(colorRef.layout);
			vkRef.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			colorRefs[i].push_back(vkRef);
		}

		/* Resolve attachments */
		for (const AttachmentReference& resolveRef : subpass.resolveAttachments) {
			VkAttachmentReference2 vkRef = { };
			vkRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
			vkRef.attachment = resolveRef.nAttachment;
			vkRef.layout = VulkanHelpers::ConvertImageLayout(resolveRef.layout);
			vkRef.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			resolveRefs[i].push_back(vkRef);
		}
		
		/* Depth stencil attachment */
		VkAttachmentReference2* pDepthRef = nullptr;
		if (subpass.bHasDepthStencil) {
			depthRefs[i].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			depthRefs[i].attachment = subpass.depthStencilAttachment.nAttachment;
			depthRefs[i].attachment = VulkanHelpers::ConvertImageLayout(subpass.depthStencilAttachment.layout);
			depthRefs[i].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			pDepthRef = &depthRefs[i];
		}

		/* Depth stencil resolve attachment */
		VkSubpassDescriptionDepthStencilResolve* pDepthResolveRef = nullptr;
		if (subpass.bHasDepthStencilResolve) {
			depthResolveRefs[i].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			depthResolveRefs[i].attachment = subpass.depthResolveAttachment.nAttachment;
			depthResolveRefs[i].attachment = VulkanHelpers::ConvertImageLayout(subpass.depthResolveAttachment.layout);
			depthResolveRefs[i].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			VkSubpassDescriptionDepthStencilResolve depthResolve = { };
			depthResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
			depthResolve.pDepthStencilResolveAttachment = &depthResolveRefs[i];
			depthResolve.stencilResolveMode = VK_RESOLVE_MODE_NONE; // Don't resolve stencil
			depthResolve.depthResolveMode = VK_RESOLVE_MODE_MAX_BIT;

			depthStencilResolves.push_back(depthResolve);
			pDepthResolveRef = &depthStencilResolves.back();
		}

		/* Subpass description */
		VkSubpassDescription2 vkSubpass = { };
		vkSubpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
		vkSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		vkSubpass.colorAttachmentCount = colorRefs[i].size();
		vkSubpass.pColorAttachments = colorRefs[i].empty() ? nullptr : colorRefs[i].data();
		vkSubpass.pResolveAttachments = resolveRefs[i].empty() ? nullptr : resolveRefs[i].data();
		vkSubpass.pDepthStencilAttachment = pDepthRef;
		vkSubpass.pNext = pDepthResolveRef;

		subpasses.push_back(vkSubpass);
	}

	/* Convert dependencies */
	Vector<VkSubpassDependency2> vkDependencies;
	for (const SubpassDependency& dependency : createInfo.dependencies) {
		VkSubpassDependency2 vkDependency = { };
		vkDependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
		vkDependency.srcSubpass = dependency.nSrcSubpass;
		vkDependency.dstSubpass = dependency.nDstSubpass;

		vkDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
									| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
									| VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		vkDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
									| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
									| VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		vkDependency.srcAccessMask = 0;
		vkDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
									| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		vkDependencies.push_back(vkDependency);
	}

	/* Create render pass */
	VkRenderPassCreateInfo2 rpInfo = { };
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
	rpInfo.attachmentCount = attachments.size();
	rpInfo.pAttachments = attachments.data();
	rpInfo.dependencyCount = vkDependencies.size();
	rpInfo.pDependencies = vkDependencies.data();
	rpInfo.subpassCount = subpasses.size();
	rpInfo.pSubpasses = subpasses.data();

	VK_CHECK(vkCreateRenderPass2(this->m_device, &rpInfo, nullptr, &this->m_renderPass), "Failed creating render pass");

	Logger::Debug(
		"VulkanRenderPass::Create: Render pass created with {} attachments and {} subpasses", 
		attachments.size(), 
		subpasses.size()
	); 
}