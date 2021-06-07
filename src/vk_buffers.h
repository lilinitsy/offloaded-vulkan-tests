#ifndef VK_BUFFERS_H
#define VK_BUFFERS_H


#include <vulkan/vulkan.h>

#include "vk_device.h"
#include "vk_initializers.h"


VkCommandBuffer begin_command_buffer(VulkanDevice device, VkCommandPool command_pool)
{
	VkCommandBufferAllocateInfo cmdbuf_ai = vki::commandBufferAllocateInfo(nullptr, command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	VkCommandBufferBeginInfo cmdbuf_bi	  = vki::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device.logical_device, &cmdbuf_ai, &command_buffer);
	vkBeginCommandBuffer(command_buffer, &cmdbuf_bi);

	return command_buffer;
}

void end_command_buffer(VulkanDevice device, VkCommandPool command_pool, VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info	   = vki::submitInfo();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers	   = &command_buffer;

	vkQueueSubmit(device.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(device.graphics_queue);
	vkFreeCommandBuffers(device.logical_device, command_pool, 1, &command_buffer);
}


void create_buffer(VulkanDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &memory)
{
	VkBufferCreateInfo buffer_ci = vki::bufferCreateInfo(size, usage, VK_SHARING_MODE_EXCLUSIVE);
	;

	VkResult create_buffer_result = vkCreateBuffer(device.logical_device, &buffer_ci, nullptr, &buffer);
	if(create_buffer_result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create buffer");
	}

	VkMemoryRequirements memreqs;
	vkGetBufferMemoryRequirements(device.logical_device, buffer, &memreqs);

	VkMemoryAllocateInfo memory_ai = vki::memoryAllocateInfo(memreqs.size, device.find_memory_type(memreqs.memoryTypeBits, properties));

	VkResult mem_alloc_result = vkAllocateMemory(device.logical_device, &memory_ai, nullptr, &memory);
	if(mem_alloc_result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate buffer memory");
	}

	vkBindBufferMemory(device.logical_device, buffer, memory, 0);
}


void copy_buffer(VulkanDevice device, VkBuffer src, VkBuffer dst, VkCommandPool command_pool, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo cmdbuf_ai = vki::commandBufferAllocateInfo(nullptr, command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

	VkCommandBuffer command_buffer = begin_command_buffer(device, command_pool);

	VkBufferCopy copy = {};
	copy.srcOffset	  = 0;
	copy.dstOffset	  = 0;
	copy.size		  = size;
	vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy); // execute the copy command on this command buffer

	end_command_buffer(device, command_pool, command_buffer);
}


void copy_buffer_to_image(VulkanDevice device, VkCommandPool command_pool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer command_buffer = begin_command_buffer(device, command_pool);

	VkImageSubresourceLayers subresource_layers	 = vki::imageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1);
	VkBufferImageCopy image_region				 = {};
	image_region.bufferOffset					 = 0;
	image_region.bufferRowLength				 = 0;
	image_region.bufferImageHeight				 = 0;
	image_region.imageSubresource.aspectMask	 = VK_IMAGE_ASPECT_COLOR_BIT,
	image_region.imageSubresource.mipLevel		 = 0;
	image_region.imageSubresource.baseArrayLayer = 0;
	image_region.imageSubresource.layerCount	 = 1;
	image_region.imageOffset					 = {0, 0, 0};
	image_region.imageExtent					 = {width, height, 1};

	vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_region);
	end_command_buffer(device, command_pool, command_buffer);
}



#endif