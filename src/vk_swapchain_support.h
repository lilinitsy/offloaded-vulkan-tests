#ifndef VK_SWAPCHAIN_SUPPORT_H
#define VK_SWAPCHAIN_SUPPORT_H

#include <vector>

#include <vulkan/vulkan.h>

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t num_formats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &num_formats, nullptr);

	if(num_formats != 0)
	{
		details.formats.resize(num_formats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &num_formats, details.formats.data());
	}

	uint32_t num_present_modes;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &num_present_modes, nullptr);

	if(num_present_modes != 0)
	{
		details.present_modes.resize(num_present_modes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &num_present_modes, details.present_modes.data());
	}

	return details;
}


#endif