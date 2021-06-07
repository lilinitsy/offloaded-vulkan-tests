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
		VkAttachmentDescription colour_attachment_description = {};
		colour_attachment_description.format				  = swapchain.format;
		colour_attachment_description.samples				  = VK_SAMPLE_COUNT_1_BIT;
		colour_attachment_description.loadOp				  = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colour_attachment_description.storeOp				  = VK_ATTACHMENT_STORE_OP_STORE;
		colour_attachment_description.stencilLoadOp			  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colour_attachment_description.stencilStoreOp		  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colour_attachment_description.initialLayout			  = VK_IMAGE_LAYOUT_UNDEFINED;
		colour_attachment_description.finalLayout			  = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colour_attachment_ref = {};
		colour_attachment_ref.attachment			= 0;
		colour_attachment_ref.layout				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Depth setup
		std::vector<VkFormat> formats						 = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
		VkAttachmentDescription depth_attachment_description = {};
		depth_attachment_description.format					 = device.find_format(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT); // depth format
		depth_attachment_description.samples				 = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment_description.loadOp					 = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment_description.storeOp				 = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Won't use it after drawing, so no need to specify storage
		depth_attachment_description.stencilLoadOp			 = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment_description.stencilStoreOp			 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment_description.initialLayout			 = VK_IMAGE_LAYOUT_UNDEFINED; // Since previous depth contents don't matter, can be undefined... may be useful to change for some AA
		depth_attachment_description.finalLayout			 = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref = {};
		depth_attachment_ref.attachment			   = 1;
		depth_attachment_ref.layout				   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass	= {};
		subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount	= 1;
		subpass.pColorAttachments		= &colour_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		std::vector<VkAttachmentDescription> attachments = {colour_attachment_description, depth_attachment_description};

		VkSubpassDependency dependency = {};
		dependency.srcSubpass		   = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass		   = 0;
		dependency.srcStageMask		   = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // fragment test for depth
		dependency.srcAccessMask	   = 0;
		dependency.dstStageMask		   = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // fragment test for depth
		dependency.dstAccessMask	   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderpass_ci = vki::renderPassCreateInfo();
		renderpass_ci.attachmentCount		 = attachments.size();
		renderpass_ci.pAttachments			 = attachments.data();
		renderpass_ci.subpassCount			 = 1;
		renderpass_ci.pSubpasses			 = &subpass;
		renderpass_ci.dependencyCount		 = 1;
		renderpass_ci.pDependencies			 = &dependency;

		if(vkCreateRenderPass(device.logical_device, &renderpass_ci, nullptr, &renderpass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass!");
		}
	}
};

#endif