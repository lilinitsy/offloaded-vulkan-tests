#ifndef DEFINES_H
#define DEFINES_H

#include <vector>

#include <vulkan/vulkan.h>

#define ENABLE_VALIDATION_LAYERS true
const uint32_t SERVERWIDTH	= 512;
const uint32_t SERVERHEIGHT = 512;

const uint32_t CLIENTWIDTH	= 1920;
const uint32_t CLIENTHEIGHT = 1080;

const uint32_t MAX_FRAMES_IN_FLIGHT = 3;

// clang-format off
const std::vector<const char*> required_validation_layers = 
{
    "VK_LAYER_KHRONOS_validation",
	//"VK_LAYER_LUNARG_api_dump",
};

const std::vector<const char*> required_device_extensions = 
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
	VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
};
// clang-format on

#endif