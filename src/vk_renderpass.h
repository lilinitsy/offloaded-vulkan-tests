#ifndef VK_RENDERPASS_H
#define VK_RENDERPASS_H


#include <vulkan/vulkan.h>

#include "vk_device.h"
#include "vk_initializers.h"
#include "vk_swapchain.h"

struct VulkanRenderpass
{
	VkRenderPass renderpass;
	std::vector<VkAttachmentDescription> attachments;
	
	VulkanRenderpass()
	{
		// don't use
	}

	VulkanRenderpass(VulkanDevice device, VulkanSwapchain swapchain, bool server)
	{
		server ? setup_renderpass(device, swapchain) : setup_client_renderpass(device, swapchain);
	}

	void setup_client_renderpass(VulkanDevice device, VulkanSwapchain swapchain)
	{
		// Swapchain image colour attachment
		VkAttachmentDescription swapchain_colour_attachment_description = {
			.format			= swapchain.format,
			.samples		= VK_SAMPLE_COUNT_1_BIT,
			.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp		= VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout	= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};


		// Input attachments - written in the first pass, transitioned to input attachments, and read in the second pass
		// Colour attachment
		VkAttachmentDescription colour_input_attachment_description = {
			.format			= swapchain.format, // may want to make this R8G8B8A8_UNORM?
			.samples		= VK_SAMPLE_COUNT_1_BIT,
			.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		// Depth attachment
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

		attachments = {swapchain_colour_attachment_description, colour_input_attachment_description, depth_attachment_description};

		std::array<VkSubpassDescription, 2> subpass_descriptions{};

		// First subpass: Writes colour and depth attachments
		VkAttachmentReference colour_reference = {
			.attachment = 1,
			.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference depth_reference = {
			.attachment = 2,
			.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		subpass_descriptions[0].pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_descriptions[0].colorAttachmentCount	= 1;
		subpass_descriptions[0].pColorAttachments		= &colour_reference;
		subpass_descriptions[0].pDepthStencilAttachment = &depth_reference;

		// Second subpass - read input attachments, and write swapchain colour attachment
		// Colour reference is the swapchain colour attachment
		VkAttachmentReference swapchain_colour_reference = {
			.attachment = 0,
			.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		// Input references
		VkAttachmentReference input_references[2];
		input_references[0] = {
			.attachment = 1,
			.layout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};
		input_references[1] = {
			.attachment = 2,
			.layout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		subpass_descriptions[1].pipelineBindPoint	 = VK_PIPELINE_BIND_POINT_GRAPHICS,
		subpass_descriptions[1].colorAttachmentCount = 1;
		subpass_descriptions[1].pColorAttachments	 = &swapchain_colour_reference;
		subpass_descriptions[1].inputAttachmentCount = 2;
		subpass_descriptions[1].pInputAttachments	 = input_references;

		// Layout transition subpass dependencies
		std::array<VkSubpassDependency, 3> dependencies;
		dependencies[0] = {
			.srcSubpass		 = VK_SUBPASS_EXTERNAL,
			.dstSubpass		 = 0,
			.srcStageMask	 = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.dstStageMask	 = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask	 = VK_ACCESS_MEMORY_READ_BIT,
			.dstAccessMask	 = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT, // framebuffer local
		};

		// Transition input attachment from colour attachment to shader read
		dependencies[1] = {
			.srcSubpass		 = 0,
			.dstSubpass		 = 1,
			.srcStageMask	 = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask	 = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask	 = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask	 = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		};

		dependencies[2] = {
			.srcSubpass		 = 0,
			.dstSubpass		 = VK_SUBPASS_EXTERNAL,
			.srcStageMask	 = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask	 = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.srcAccessMask	 = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask	 = VK_ACCESS_MEMORY_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		};

		VkRenderPassCreateInfo renderpass_ci = vki::renderPassCreateInfo();
		renderpass_ci.attachmentCount		 = attachments.size();
		renderpass_ci.pAttachments			 = attachments.data();
		renderpass_ci.subpassCount			 = subpass_descriptions.size();
		renderpass_ci.pSubpasses			 = subpass_descriptions.data();
		renderpass_ci.dependencyCount		 = dependencies.size();
		renderpass_ci.pDependencies			 = dependencies.data();

		VkResult renderpass_creation = vkCreateRenderPass(device.logical_device, &renderpass_ci, nullptr, &renderpass);
		if(renderpass_creation != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void setup_renderpass(VulkanDevice device, VulkanSwapchain swapchain)
	{
		// Swapchain image colour attachment
		VkAttachmentDescription swapchain_colour_attachment_description = {
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

		attachments = {swapchain_colour_attachment_description, depth_attachment_description};

		VkSubpassDependency dependency = {
			.srcSubpass	   = VK_SUBPASS_EXTERNAL,
			.dstSubpass	   = 0,
			.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, // fragment test for depth
			.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, // fragment test for depth
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		};

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