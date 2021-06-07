#ifndef VK_DEVICE_H
#define VK_DEVICE_H

#include <cstring>
#include <set>
#include <stdexcept>
#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "defines.h"
#include "vk_initializers.h"
#include "vk_queuefamilies.h"
#include "vk_swapchain_support.h"

struct VulkanDevice
{
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice logical_device;
	VkQueue graphics_queue;
	VkQueue present_queue;

	VulkanDevice()
	{
		// Don't use this
	}

	VulkanDevice(VkInstance instance, VkSurfaceKHR surface)
	{
		select_physical_device(instance, surface);
		initialize_logical_device(surface);
	}

	void select_physical_device(VkInstance instance, VkSurfaceKHR surface)
	{
		uint32_t num_devices = 0;
		vkEnumeratePhysicalDevices(instance, &num_devices, nullptr);

		if(num_devices == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(num_devices);
		vkEnumeratePhysicalDevices(instance, &num_devices, devices.data());

		for(uint32_t i = 0; i < devices.size(); i++)
		{
			if(device_valid(devices[i], surface))
			{
				physical_device = devices[i];
				break;
			}
		}

		if(physical_device == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	void initialize_logical_device(VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices = search_queue_families(physical_device, surface);

		std::vector<VkDeviceQueueCreateInfo> device_queue_ci;
		std::set<uint32_t> unique_queue_families = {indices.graphics_qf, indices.present_qf};

		float queue_priority = 1.0f;
		for(uint32_t queueFamily : unique_queue_families)
		{
			VkDeviceQueueCreateInfo queue_ci = vki::deviceQueueCreateInfo(queueFamily, 1, &queue_priority);
			device_queue_ci.push_back(queue_ci);
		}

		VkPhysicalDeviceFeatures device_features = {};
		device_features.samplerAnisotropy		 = VK_TRUE;
		VkDeviceCreateInfo logical_device_ci	 = vki::deviceCreateInfo(device_queue_ci.size(), device_queue_ci.data(), required_validation_layers.size(), required_validation_layers.data(), required_device_extensions.size(), required_device_extensions.data(), &device_features);


		if(vkCreateDevice(physical_device, &logical_device_ci, nullptr, &logical_device) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(logical_device, indices.graphics_qf, 0, &graphics_queue);
		vkGetDeviceQueue(logical_device, indices.present_qf, 0, &present_queue);
	}


	bool device_valid(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices = search_queue_families(device, surface);

		bool extensions_supported = check_device_extensions_supported(device);

		bool swapchain_supported = false;
		if(extensions_supported)
		{
			SwapChainSupportDetails swapchain_support_details = query_swapchain_support(device, surface);
			swapchain_supported								  = !swapchain_support_details.formats.empty() && !swapchain_support_details.present_modes.empty();
		}

		VkPhysicalDeviceFeatures features_supported;
		vkGetPhysicalDeviceFeatures(device, &features_supported);

		return indices.qf_completed() && extensions_supported && swapchain_supported && features_supported.samplerAnisotropy;
	}

	bool check_device_extensions_supported(VkPhysicalDevice device)
	{
		uint32_t num_extensions;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, nullptr);

		std::vector<VkExtensionProperties> available_extensions(num_extensions);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, available_extensions.data());

		std::set<std::string> required_extensions(required_device_extensions.begin(), required_device_extensions.end());

		for(VkExtensionProperties extension : available_extensions)
		{
			required_extensions.erase(extension.extensionName);
		}

		return required_extensions.empty();
	}


	uint32_t find_memory_type(uint32_t filter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memprops;
		vkGetPhysicalDeviceMemoryProperties(physical_device, &memprops);

		for(uint32_t i = 0; i < memprops.memoryTypeCount; i++)
		{
			if(filter & (1 << i) && (memprops.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("Could not find a compatible memory type for buffer");
	}

	VkFormat find_format(std::vector<VkFormat> formats, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for(uint32_t i = 0; i < formats.size(); i++)
		{
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(physical_device, formats[i], &properties);

			if((tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) || (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features))
			{
				return formats[i];
			}
		}

		throw std::runtime_error("Could not find a valid format for the device");
	}


	void destroy()
	{
		vkDestroyDevice(logical_device, nullptr);
	}
};


std::vector<const char *> find_required_extensions()
{
	uint32_t glfw_extension_count = 0;
	const char **glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if(ENABLE_VALIDATION_LAYERS)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool check_validation_layer_support()
{
	uint32_t num_layers;
	vkEnumerateInstanceLayerProperties(&num_layers, nullptr);

	std::vector<VkLayerProperties> available_layers(num_layers);
	vkEnumerateInstanceLayerProperties(&num_layers, available_layers.data());

	for(uint32_t i = 0; i < required_validation_layers.size(); i++)
	{
		bool layer_found = false;

		for(uint32_t j = 0; j < available_layers.size(); j++)
		{
			if(strcmp(required_validation_layers[i], available_layers[j].layerName) == 0)
			{
				layer_found = true;
				break;
			}
		}

		if(!layer_found)
		{
			return false;
		}
	}

	return true;
}



#endif
