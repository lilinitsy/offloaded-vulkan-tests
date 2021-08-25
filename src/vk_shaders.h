#ifndef VK_SHADERS_H
#define VK_SHADERS_H

#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "vk_device.h"
#include "vk_initializers.h"

#include "defines.h"


struct UBO
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

struct UBOClient
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;

	// Viewport
	/*float x		 = CLIENTWIDTH / 2 - SERVERWIDTH / 2; // x and y are viewport's upper left corner (x, y)
	float y		 = CLIENTWIDTH / 2 - SERVERWIDTH / 2;
	float width	 = SERVERWIDTH; // width and height are viewport's width and height
	float height = SERVERWIDTH;*/
};


VkShaderModule setup_shader_module(const std::vector<char> &code, VulkanDevice device)
{
	VkShaderModuleCreateInfo shader_module_ci = vki::shaderModuleCreateInfo(code.size(), reinterpret_cast<const uint32_t *>(code.data()));

	VkShaderModule shader_module;
	if(vkCreateShaderModule(device.logical_device, &shader_module_ci, nullptr, &shader_module) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}

	return shader_module;
}


#endif