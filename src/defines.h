#ifndef DEFINES_H
#define DEFINES_H

#include <vector>

#include <vulkan/vulkan.h>

#define ENABLE_VALIDATION_LAYERS true
const uint32_t SERVERWIDTH	= 1920;
const uint32_t SERVERHEIGHT = 1080;

const uint32_t CLIENTWIDTH	= 1920;
const uint32_t CLIENTHEIGHT = 1080;

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

// clang-format off
const std::vector<const char*> required_validation_layers = 
{
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> required_device_extensions = 
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
// clang-format on

#endif