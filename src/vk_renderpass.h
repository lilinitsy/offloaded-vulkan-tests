#ifndef VK_RENDERPASS_H
#define VK_RENDERPASS_H


#include <vulkan/vulkan.h>

#include "vk_device.h"
#include "vk_initializers.h"
#include "vk_swapchain.h"

struct VulkanRenderpass
{
	VkRenderPass renderpass;

	VulkanRenderpass()
	{
		// don't use
	}

	VulkanRenderpass(VulkanDevice device, VulkanSwapchain swapchain)
	{
		setup_renderpass(device, swapchain);
	}

	void setup_renderpass(VulkanDevice device, VulkanSwapchain swapchain)
	{
		VkAttachmentDescription colour_attachment_description = {
			.format			= swapchain.format,
			.samples		= VK_SAMPLE_COUNT_1_BIT,
			.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp		= VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		VkAttachmentReference colour_attachment_ref = {
			.attachment = 0,
			.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		// Depth setup
		std::vector<VkFormat> formats						 = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
		VkAttachmentDescription depth_attachment_description = {
			.format			= device.find_format(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT), // depth format
			.samples		= VK_SAMPLE_COUNT_1_BIT,
			.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE, // Won't use it after drawing, so no need to specify storage
			.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED, // Since previous depth contents don't matter, can be undefined... may be useful to change for some AA
			.finalLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference depth_attachment_ref = {
			.attachment = 1,
			.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass = {
			.pipelineBindPoint		 = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount	 = 1,
			.pColorAttachments		 = &colour_attachment_ref,
			.pDepthStencilAttachment = &depth_attachment_ref,
		};

		std::vector<VkAttachmentDescription> attachments = {colour_attachment_description, depth_attachment_description};

		VkSubpassDependency dependencies[2];


		dependencies[0] = {
			.srcSubpass	   = VK_SUBPASS_EXTERNAL,
			.dstSubpass	   = 0,
			.srcStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		};

		dependencies[1] = {
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		};


		////// From sascha willems' multiview setup
		/*
			Bit mask that specifies which view rendering is broadcast to
			0011 = Broadcast to first and second view (layer)
		*/
		uint32_t view_mask = 0b00000011;

		/*
			Bit mask that specifies correlation between views
			An implementation may use this for optimizations (concurrent render)
		*/
		uint32_t correlation_mask = 0b00000011;

		VkRenderPassMultiviewCreateInfo renderpass_multiview_ci = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO,
			.subpassCount = 1,
			.pViewMasks = &view_mask,
			.correlationMaskCount = 1,
			.pCorrelationMasks = &correlation_mask,
		};

		VkRenderPassCreateInfo renderpass_ci = vki::renderPassCreateInfo();
		renderpass_ci.attachmentCount		 = attachments.size();
		renderpass_ci.pAttachments			 = attachments.data();
		renderpass_ci.subpassCount			 = 1;
		renderpass_ci.pSubpasses			 = &subpass;
		renderpass_ci.dependencyCount		 = 2;
		renderpass_ci.pDependencies			 = dependencies;
		renderpass_ci.pNext = &renderpass_multiview_ci;



		if(vkCreateRenderPass(device.logical_device, &renderpass_ci, nullptr, &renderpass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass!");
		}
	}
};

struct VulkanRenderpassClient
{
	VkRenderPass renderpass;

	VulkanRenderpassClient()
	{
		// don't use this
	}

	VulkanRenderpassClient(VulkanDevice device, VulkanSwapchain swapchain)
	{
		setup_renderpass(device, swapchain);
	}


	void setup_renderpass(VulkanDevice device, VulkanSwapchain swapchain)
	{
		VkAttachmentDescription colour_attachment_description = {
			.format			= swapchain.format,
			.samples		= VK_SAMPLE_COUNT_1_BIT,
			.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp		= VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout	= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		VkAttachmentReference colour_attachment_ref = {
			.attachment = 0,
			.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		// Depth setup
		std::vector<VkFormat> formats						 = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
		VkAttachmentDescription depth_attachment_description = {
			.format			= device.find_format(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT), // depth format
			.samples		= VK_SAMPLE_COUNT_1_BIT,
			.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE, // Won't use it after drawing, so no need to specify storage
			.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED, // Since previous depth contents don't matter, can be undefined... may be useful to change for some AA
			.finalLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference depth_attachment_ref = {
			.attachment = 1,
			.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass = {
			.pipelineBindPoint		 = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount	 = 1,
			.pColorAttachments		 = &colour_attachment_ref,
			.pDepthStencilAttachment = &depth_attachment_ref,
		};

		std::vector<VkAttachmentDescription> attachments = {colour_attachment_description, depth_attachment_description};

		// layout transitions
		std::array<VkSubpassDependency, 2> dependencies;
		dependencies[0] = {
			.srcSubpass		 = VK_SUBPASS_EXTERNAL,
			.dstSubpass		 = 0,
			.srcStageMask	 = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask	 = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask	 = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask	 = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		};

		dependencies[1] = {
			.srcSubpass		 = 0,
			.dstSubpass		 = VK_SUBPASS_EXTERNAL,
			.srcStageMask	 = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask	 = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask	 = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask	 = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		};

		VkRenderPassCreateInfo renderpass_ci = vki::renderPassCreateInfo();
		renderpass_ci.attachmentCount		 = attachments.size();
		renderpass_ci.pAttachments			 = attachments.data();
		renderpass_ci.subpassCount			 = 1;
		renderpass_ci.pSubpasses			 = &subpass;
		renderpass_ci.dependencyCount		 = dependencies.size();
		renderpass_ci.pDependencies			 = dependencies.data();

		VkResult renderpass_create = vkCreateRenderPass(device.logical_device, &renderpass_ci, nullptr, &renderpass);
	}
};

#endif