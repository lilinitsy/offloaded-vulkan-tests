#ifndef VK_IMAGE_H
#define VK_IMAGE_H


#include <stdexcept>

#include <vulkan/vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#include "vk_buffers.h"
#include "vk_device.h"


struct ImagePacket
{
	VkImage image;
	VkDeviceMemory memory;
	VkSubresourceLayout subresource_layout;
	char *data;

	void map_memory(VulkanDevice device)
	{
		vkMapMemory(device.logical_device, memory, 0, VK_WHOLE_SIZE, 0, (void **) &data);
		data += subresource_layout.offset;
		vkUnmapMemory(device.logical_device, memory);
	}

	void destroy(VulkanDevice device)
	{
		vkDestroyImage(device.logical_device, image, nullptr);
		vkFreeMemory(device.logical_device, memory, nullptr);
	}
};


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

void transition_image_layout(VulkanDevice device, VkCommandPool command_pool, VkCommandBuffer command_buffer, VkImage image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask)
{
	VkImageSubresourceRange subresource_range = vki::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
	VkImageMemoryBarrier barrier			  = vki::imageMemoryBarrier(src_access_mask, dst_access_mask, old_layout, new_layout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image, subresource_range);

	// the pipeline stage to submit, pipeline stage to wait on
	vkCmdPipelineBarrier(command_buffer, src_stage_mask, dst_stage_mask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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

VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, bool t)
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
	std::cout << "image view: " << image_view << std::endl;
	VkResult image_view_create = vkCreateImageView(device, &image_view_ci, nullptr, &image_view);
	if(image_view_create != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create image views");
	}

	return image_view;
}


	ImagePacket copy_image_to_packet(VulkanDevice device, VkCommandPool command_pool, VkImage src_image)
	{
		timeval timer_start;
		timeval timer_end;
		gettimeofday(&timer_start, nullptr);

		// Use the most recently rendered swapchain image as the source
		VkCommandBuffer copy_cmdbuffer = begin_command_buffer(device, command_pool);

		// Create the destination image that will be copied to -- not sure this is actually gonna be necessary to stream?
		ImagePacket dst;
		VkExtent3D extent = {SERVERWIDTH, SERVERHEIGHT, 1};
		create_image(device, 0, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SNORM, extent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_IMAGE_LAYOUT_UNDEFINED, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, dst.image, dst.memory);

		// Blit from the swapchain image to the copied image
		// Transition dst image to destination layout
		transition_image_layout(device, command_pool, copy_cmdbuffer,
								dst.image,
								0,
								VK_ACCESS_TRANSFER_WRITE_BIT,
								VK_IMAGE_LAYOUT_UNDEFINED,
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_TRANSFER_BIT);

		// Transition swapchain image from present to source's transfer layout
		transition_image_layout(device, command_pool, copy_cmdbuffer,
								src_image,
								VK_ACCESS_MEMORY_READ_BIT,
								VK_ACCESS_TRANSFER_READ_BIT,
								VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
								VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_TRANSFER_BIT);


		// Copy the image
		VkImageCopy image_copy_region				= {}; // For some reason, using the vki functions on subresources isn't working
		image_copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_copy_region.srcSubresource.layerCount = 1;
		image_copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_copy_region.dstSubresource.layerCount = 1;
		image_copy_region.extent.width				= SERVERWIDTH;
		image_copy_region.extent.height				= SERVERHEIGHT;
		image_copy_region.extent.depth				= 1;

		vkCmdCopyImage(copy_cmdbuffer,
					   src_image,
					   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   dst.image,
					   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   1,
					   &image_copy_region);


		// Transition dst image to general layout -- lets us map the image memory
		transition_image_layout(device, command_pool, copy_cmdbuffer,
								dst.image,
								VK_ACCESS_TRANSFER_WRITE_BIT,
								VK_ACCESS_MEMORY_READ_BIT,
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								VK_IMAGE_LAYOUT_GENERAL,
								VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_TRANSFER_BIT);


		// transition to swapchain image now that copying is done
		transition_image_layout(device, command_pool, copy_cmdbuffer,
								src_image,
								VK_ACCESS_TRANSFER_READ_BIT,
								VK_ACCESS_MEMORY_READ_BIT,
								VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
								VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_PIPELINE_STAGE_TRANSFER_BIT);

		end_command_buffer(device, command_pool, copy_cmdbuffer);

		VkImageSubresource subresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
		vkGetImageSubresourceLayout(device.logical_device, dst.image, &subresource, &dst.subresource_layout);

		dst.map_memory(device);

		gettimeofday(&timer_end, nullptr);

		double dt = timer_end.tv_sec - timer_start.tv_sec + (timer_end.tv_usec - timer_start.tv_usec);
		//printf("dt: %f\n", (dt / 1000000.0f));

		return dst;
	}



#endif
