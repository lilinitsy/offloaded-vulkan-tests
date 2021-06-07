#ifndef VK_QUEUEFAMILIES_H
#define VK_QUEUEFAMILIES_H

#include <vector>

#include <vulkan/vulkan.h>

struct QueueFamilyIndices
{
	uint32_t graphics_qf;
	uint32_t present_qf;

	bool qf_completed()
	{
		return graphics_qf >= 0 && present_qf >= 0;
	}
};

QueueFamilyIndices search_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;

	uint32_t num_qfs = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &num_qfs, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(num_qfs);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &num_qfs, queue_families.data());

	int queue_index = 0;
	for(uint32_t i = 0; i < queue_families.size(); i++)
	{
		if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphics_qf = queue_index;
		}

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_index, surface, &present_support);

		if(present_support)
		{
			indices.present_qf = queue_index;
		}

		if(indices.qf_completed())
		{
			break;
		}

		queue_index++;
	}

	return indices;
}

#endif