#ifndef VK_SWAPCHAIN_H
#define VK_SWAPCHAIN_H

#include <stdexcept>
#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "vk_initializers.h"
#include "vk_queuefamilies.h"
#include "vk_swapchain_support.h"
#include "vk_device.h"



struct VulkanSwapchain
{
	VkSwapchainKHR swapchain;
	std::vector<VkImage> images;
	VkFormat format;
	VkExtent2D swapchain_extent;
	std::vector<VkImageView> image_views;
	std::vector<VkSampler> samplers;
	std::vector<VkFramebuffer> framebuffers;

	VulkanSwapchain()
	{
		// don't use this
	}

	VulkanSwapchain(SwapChainSupportDetails swapchain_support, VkSurfaceKHR surface, VulkanDevice device, GLFWwindow *window)
	{
		setup_swapchain(swapchain_support, surface, device, window);
		setup_image_views(device.logical_device);
	}

	void setup_swapchain(SwapChainSupportDetails swapchain_support, VkSurfaceKHR surface, VulkanDevice device, GLFWwindow *window)
	{
		VkSurfaceFormatKHR surface_format = select_swapchain_surface_format(swapchain_support.formats);
		VkPresentModeKHR present_mode	  = select_swapchain_present_mode(swapchain_support.present_modes);
		VkExtent2D extent				  = select_swapchain_extent(swapchain_support.capabilities, window);

		uint32_t num_images = swapchain_support.capabilities.minImageCount + 1;
		if(swapchain_support.capabilities.maxImageCount > 0 && num_images > swapchain_support.capabilities.maxImageCount)
		{
			num_images = swapchain_support.capabilities.maxImageCount;
		}

		QueueFamilyIndices indices		   = search_queue_families(device.physical_device, surface);
		uint32_t qf_indices[]			   = {indices.graphics_qf, indices.present_qf};
		bool graphics_qf_equals_present_qf = indices.graphics_qf == indices.present_qf;

		VkSwapchainCreateInfoKHR swapchain_ci = vki::swapchainCreateInfoKHR();
		swapchain_ci.surface				  = surface;
		swapchain_ci.minImageCount			  = num_images;
		swapchain_ci.imageFormat			  = surface_format.format;
		swapchain_ci.imageColorSpace		  = surface_format.colorSpace;
		swapchain_ci.imageExtent			  = extent;
		swapchain_ci.imageArrayLayers		  = 1;
		swapchain_ci.imageUsage				  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchain_ci.imageSharingMode		  = graphics_qf_equals_present_qf ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
		swapchain_ci.queueFamilyIndexCount	  = graphics_qf_equals_present_qf ? 0 : 2;
		swapchain_ci.pQueueFamilyIndices	  = graphics_qf_equals_present_qf ? nullptr : qf_indices;
		swapchain_ci.preTransform			  = swapchain_support.capabilities.currentTransform;
		swapchain_ci.compositeAlpha			  = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_ci.presentMode			  = present_mode;
		swapchain_ci.clipped				  = VK_TRUE;
		swapchain_ci.oldSwapchain			  = VK_NULL_HANDLE;

		if(vkCreateSwapchainKHR(device.logical_device, &swapchain_ci, nullptr, &swapchain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device.logical_device, swapchain, &num_images, nullptr);
		images.resize(num_images);
		vkGetSwapchainImagesKHR(device.logical_device, swapchain, &num_images, images.data());

		format			 = surface_format.format;
		swapchain_extent = extent;
	}


	void setup_image_views(VkDevice device)
	{
		image_views.resize(images.size());

		for(uint32_t i = 0; i < images.size(); i++)
		{
			image_views[i] = create_image_view(device, images[i], format, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void setup_samplers(VulkanDevice device)
	{
		samplers.resize(images.size());

		for(uint32_t i = 0; i < images.size(); i++)
		{
			// Create the sampler
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(device.physical_device, &properties);
			VkSamplerCreateInfo sampler_ci = vki::samplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
																	VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
																	0.0f, VK_TRUE, properties.limits.maxSamplerAnisotropy, VK_FALSE, VK_COMPARE_OP_ALWAYS,
																	0.0f, 0.0f, VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_FALSE);

			VkResult sampler_create = vkCreateSampler(device.logical_device, &sampler_ci, nullptr, &samplers[i]);
			if(sampler_create != VK_SUCCESS)
			{
				throw std::runtime_error("Could not create texture sampler");
			}
		}
	}


	VkSurfaceFormatKHR select_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &available_surface_formats)
	{
		for(uint32_t i = 0; i < available_surface_formats.size(); i++)
		{
			if(available_surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && available_surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return available_surface_formats[i];
			}
		}

		return available_surface_formats[0];
	}

	VkPresentModeKHR select_swapchain_present_mode(const std::vector<VkPresentModeKHR> &available_present_modes)
	{
		for(uint32_t i = 0; i < available_present_modes.size(); i++)
		{
			if(available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return available_present_modes[i];
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D select_swapchain_extent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window)
	{
		if(capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actual_extent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)};

			actual_extent.width	 = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
			actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));

			return actual_extent;
		}
	}
};


struct VulkanAttachment
{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView image_view;
};

void destroy_vulkan_attachment(VkDevice logical_device, VulkanAttachment attachment)
{
	vkDestroyImageView(logical_device, attachment.image_view, nullptr);
	vkDestroyImage(logical_device, attachment.image, nullptr);
	vkFreeMemory(logical_device, attachment.memory, nullptr);
}


#endif