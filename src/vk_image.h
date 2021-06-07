#ifndef VK_IMAGE_H
#define VK_IMAGE_H


#include <stdexcept>

#include <vulkan/vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#include "vk_buffers.h"
#include "vk_device.h"


void transition_image_layout(VulkanDevice device, VkCommandPool command_pool, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkCommandBuffer command_buffer			  = begin_command_buffer(device, command_pool);
	VkImageSubresourceRange subresource_range = vki::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
	VkImageMemoryBarrier barrier			  = vki::imageMemoryBarrier(0, 0, old_layout, new_layout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image, subresource_range);

	VkPipelineStageFlags src_stage;
	VkPipelineStageFlags dst_stage;

	if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		src_stage			  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage			  = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	else if(old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		src_stage			  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dst_stage			  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}

	// the pipeline stage to submit, pipeline stage to wait on
	vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	end_command_buffer(device, command_pool, command_buffer);
}


void create_image(VulkanDevice device, VkImageCreateFlags flags, VkImageType image_type, VkFormat format, VkExtent3D extent, uint32_t mip_levels, uint32_t array_layers, VkSampleCountFlagBits samples, VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharing_mode, VkImageLayout initial_layout, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory)
{
	VkImageCreateInfo image_ci	 = vki::imageCreateInfo(flags, image_type, format, extent, mip_levels, array_layers, samples, tiling, usage, sharing_mode, initial_layout);
	VkResult create_image_result = vkCreateImage(device.logical_device, &image_ci, nullptr, &image);
	if(create_image_result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create a VkImage");
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device.logical_device, image, &memory_requirements);

	VkMemoryAllocateInfo mem_ai = vki::memoryAllocateInfo(memory_requirements.size, device.find_memory_type(memory_requirements.memoryTypeBits, properties));
	VkResult mem_alloc_result	= vkAllocateMemory(device.logical_device, &mem_ai, nullptr, &image_memory);
	if(mem_alloc_result != VK_SUCCESS)
	{
		throw std::runtime_error("could not allocate image memory");
	}

	vkBindImageMemory(device.logical_device, image, image_memory, 0);
}

VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo image_view_ci			  = vki::imageViewCreateInfo();
	image_view_ci.image							  = image;
	image_view_ci.viewType						  = VK_IMAGE_VIEW_TYPE_2D;
	image_view_ci.format						  = format;
	image_view_ci.subresourceRange.aspectMask	  = aspectFlags;
	image_view_ci.subresourceRange.baseMipLevel	  = 0;
	image_view_ci.subresourceRange.levelCount	  = 1;
	image_view_ci.subresourceRange.baseArrayLayer = 0;
	image_view_ci.subresourceRange.layerCount	  = 1;

	VkImageView image_view;
	VkResult image_view_create = vkCreateImageView(device, &image_view_ci, nullptr, &image_view);
	if(image_view_create != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create image views");
	}

	return image_view;
}


#endif