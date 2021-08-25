#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <set>
#include <stdexcept>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#define GLM_FORCE_RADIANS
#define GLMFORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>

#include "camera.h"
#include "defines.h"
#include "utils.h"
#include "vertex.h"
#include "vk_debug_messenger.h"
#include "vk_device.h"
#include "vk_image.h"
#include "vk_models.h"
#include "vk_queuefamilies.h"
#include "vk_renderpass.h"
#include "vk_shaders.h"
#include "vk_swapchain.h"
#include "vk_swapchain_support.h"

std::string MODEL_PATH	 = "../models/labdesk/labdesk.obj";
std::string TEXTURE_PATH = "../models/labdesk/labdesk.jpg";


#define PORT 1234

Camera camera = Camera(glm::vec3(0.0f, 2.0f, 2.0f));

struct
{
	float last_x;
	float last_y;
	float yaw;
	float pitch;
	bool fmouse;
} mouse;


void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	if(mouse.fmouse)
	{
		mouse.last_x = xpos;
		mouse.last_y = ypos;
		mouse.fmouse = false;
	}

	float xoffset = xpos - mouse.last_x;
	float yoffset = mouse.last_y - ypos;
	mouse.last_x  = xpos;
	mouse.last_y  = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	mouse.yaw += xoffset;
	mouse.pitch += yoffset;

	if(mouse.pitch > 89.0f)
	{
		mouse.pitch = 89.0f;
	}

	if(mouse.pitch < -89.0f)
	{
		mouse.pitch = -89.0f;
	}

	glm::vec3 front;
	front.x		 = cos(glm::radians(mouse.yaw)) * cos(glm::radians(mouse.pitch));
	front.y		 = sin(glm::radians(mouse.pitch));
	front.z		 = sin(glm::radians(mouse.yaw)) * cos(glm::radians(mouse.pitch));
	camera.front = glm::normalize(front);
}


struct Client
{
	int socket_fd;

	Client()
	{
		socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		if(socket_fd == -1)
		{
			throw std::runtime_error("Could not create a socket");
		}
	}

	void connect_to_server(int port)
	{
		sockaddr_in server_address = {
			.sin_family = AF_INET,
			.sin_port	= htons(static_cast<in_port_t>(port)),
			.sin_addr	= htonl(0xc0a80002),
		};


		int connect_result = connect(socket_fd, (sockaddr *) &server_address, sizeof(server_address));
		if(connect_result == -1)
		{
			throw std::runtime_error("Could not connect to server");
		}
	}
};


struct DeviceRenderer
{
	void run()
	{
		initWindow();
		init_vulkan();
		game_loop();
		cleanup();
	}

	GLFWwindow *window;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkSurfaceKHR surface;

	VulkanDevice device;
	VulkanSwapchain swapchain;
	VulkanRenderpass renderpass;

	struct PipelineLayouts
	{
		VkPipelineLayout model;
		VkPipelineLayout fsquad;
	} pipeline_layouts;

	struct Pipelines
	{
		VkPipeline model;
		VkPipeline fsquad;
	} pipelines;

	Model model;
	VkBuffer vbo;
	VkBuffer ibo;
	std::vector<VkBuffer> ubos;
	std::vector<VkDeviceMemory> ubos_mem;
	VkDeviceMemory vbo_mem;
	VkDeviceMemory ibo_mem;

	VulkanAttachment server_colour_attachment;
	VkSampler server_frame_sampler;

	VulkanAttachment depth_attachment;

	VulkanAttachment texcolour_attachment;
	VkSampler tex_sampler;

	// Buffer that will be copied to
	VkBuffer image_buffer;
	VkDeviceMemory image_buffer_memory;

	VkCommandPool command_pool;
	std::vector<VkCommandBuffer> command_buffers;

	struct
	{
		VkDescriptorSetLayout model;
		VkDescriptorSetLayout fsquad;
	} descriptor_set_layouts;

	VkDescriptorPool descriptor_pool;

	struct DescriptorSets
	{
		std::vector<VkDescriptorSet> model;
		std::vector<VkDescriptorSet> fsquad;
	} descriptor_sets;


	// Modeled from sascha's radialblur
	struct OffscreenPass
	{
		VkFramebuffer framebuffer;
		VulkanAttachment colour_attachment;
		VulkanAttachment depth_attachment;
		VkRenderPass renderpass;
		VkSampler sampler;
	} offscreen_pass;


	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;
	std::vector<VkFence> images_in_flight;
	uint32_t current_frame = 0;
	uint64_t numframes	   = 0;

	struct
	{
		pthread_t rec_image_thread;
		pthread_t first_renderpass_thread;
	} vk_pthread_t;

	Client client;

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(CLIENTWIDTH, CLIENTHEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetCursorPosCallback(window, mouse_callback);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	void init_vulkan()
	{
		setup_instance();
		setupDebugMessenger(instance, &debug_messenger);
		setup_surface();
		device									  = VulkanDevice(instance, surface);
		SwapChainSupportDetails swapchain_support = query_swapchain_support(device.physical_device, surface);
		swapchain								  = VulkanSwapchain(swapchain_support, surface, device, window);
		renderpass								  = VulkanRenderpass(device, swapchain);
		setup_offscreen();
		setup_descriptor_set_layout();
		setup_graphics_pipeline();
		setup_command_pool();
		setup_depth();
		setup_framebuffers();
		setup_serverframe_sampler();
		setup_texture_sampler();
		model = Model(MODEL_PATH, TEXTURE_PATH, glm::vec3(-1.0f, -0.5f, 0.5f));
		initialize_vertex_buffers(device, model.vertices, &vbo, &vbo_mem, command_pool);
		initialize_index_buffers(device, model.indices, &ibo, &ibo_mem, command_pool);
		initialize_ubos();
		setup_descriptor_pool();
		setup_descriptor_sets();
		//setup_command_buffers();
		setup_vk_async();

		create_copy_image_buffer();

		client = Client();
		client.connect_to_server(PORT);
	}

	void game_loop()
	{
		while(!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			render_complete_frame();
		}

		vkDeviceWaitIdle(device.logical_device);
	}

	void cleanup()
	{
		cleanup_swapchain();
		vkDestroyImageView(device.logical_device, server_colour_attachment.image_view, nullptr);
		vkDestroySampler(device.logical_device, server_frame_sampler, nullptr);
		vkDestroyImage(device.logical_device, server_colour_attachment.image, nullptr);
		vkFreeMemory(device.logical_device, server_colour_attachment.memory, nullptr);

		vkDestroyDescriptorSetLayout(device.logical_device, descriptor_set_layouts.model, nullptr);

		vkDestroyBuffer(device.logical_device, vbo, nullptr);
		vkDestroyBuffer(device.logical_device, ibo, nullptr);
		vkFreeMemory(device.logical_device, vbo_mem, nullptr);
		vkFreeMemory(device.logical_device, ibo_mem, nullptr);

		for(uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device.logical_device, render_finished_semaphores[i], nullptr);
			vkDestroySemaphore(device.logical_device, image_available_semaphores[i], nullptr);
			vkDestroyFence(device.logical_device, in_flight_fences[i], nullptr);
		}

		vkDestroyCommandPool(device.logical_device, command_pool, nullptr);

		for(uint32_t i = 0; i < swapchain.framebuffers.size(); i++)
		{
			vkDestroyFramebuffer(device.logical_device, swapchain.framebuffers[i], nullptr);
		}

		vkDestroyBuffer(device.logical_device, vbo, nullptr);
		vkFreeMemory(device.logical_device, vbo_mem, nullptr);
		device.destroy();

		// Destroy image buffer
		vkDestroyBuffer(device.logical_device, image_buffer, nullptr);
		vkFreeMemory(device.logical_device, image_buffer_memory, nullptr);

		if(ENABLE_VALIDATION_LAYERS)
		{
			DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);


		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void cleanup_swapchain()
	{
		for(uint32_t i = 0; i < swapchain.framebuffers.size(); i++)
		{
			vkDestroyFramebuffer(device.logical_device, swapchain.framebuffers[i], nullptr);
		}

		for(uint32_t i = 0; i < swapchain.images.size(); i++)
		{
			vkDestroyBuffer(device.logical_device, ubos[i], nullptr);
			vkFreeMemory(device.logical_device, ubos_mem[i], nullptr);
		}

		vkDestroyDescriptorPool(device.logical_device, descriptor_pool, nullptr);

		vkFreeCommandBuffers(device.logical_device, command_pool, command_buffers.size(), command_buffers.data());
		vkDestroyPipeline(device.logical_device, pipelines.model, nullptr);
		vkDestroyPipelineLayout(device.logical_device, pipeline_layouts.model, nullptr);
		vkDestroyRenderPass(device.logical_device, renderpass.renderpass, nullptr);

		for(uint32_t i = 0; i < swapchain.image_views.size(); i++)
		{
			vkDestroyImageView(device.logical_device, swapchain.image_views[i], nullptr);
		}

		for(uint32_t i = 0; i < swapchain.images.size(); i++)
		{
			vkDestroyBuffer(device.logical_device, ubos[i], nullptr);
			vkFreeMemory(device.logical_device, ubos_mem[i], nullptr);
		}

		vkDestroySwapchainKHR(device.logical_device, swapchain.swapchain, nullptr);
	}

	void setup_instance()
	{
		VkApplicationInfo app_info	= {};
		app_info.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName	= "Offloaded Rendering Test";
		app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
		app_info.pEngineName		= "";
		app_info.engineVersion		= VK_MAKE_VERSION(0, 0, 1);
		app_info.apiVersion			= VK_API_VERSION_1_2;

		VkDebugUtilsMessengerCreateInfoEXT debug_setup_info;
		populateDebugMessengerCreateInfo(debug_setup_info);
		std::vector<const char *> extensions = find_required_extensions();

		VkInstanceCreateInfo instance_ci	= vki::instanceCreateInfo();
		instance_ci.pApplicationInfo		= &app_info;
		instance_ci.enabledExtensionCount	= static_cast<uint32_t>(extensions.size());
		instance_ci.ppEnabledExtensionNames = extensions.data();
		instance_ci.enabledLayerCount		= static_cast<uint32_t>(required_validation_layers.size());
		instance_ci.ppEnabledLayerNames		= required_validation_layers.data();

		instance_ci.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debug_setup_info;

		if(vkCreateInstance(&instance_ci, nullptr, &instance) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance!");
		}
	}


	void setup_surface()
	{
		if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}


	void setup_offscreen()
	{
		// Depth attachment
		std::vector<VkFormat> formats = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
		VkFormat depth_format		  = device.find_format(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		VkExtent3D extent			  = {swapchain.swapchain_extent.width, swapchain.swapchain_extent.height, 1};

		create_image(device, 0, VK_IMAGE_TYPE_2D, depth_format, extent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_IMAGE_LAYOUT_UNDEFINED, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreen_pass.depth_attachment.image, offscreen_pass.depth_attachment.memory);
		offscreen_pass.depth_attachment.image_view = create_image_view(device.logical_device, offscreen_pass.depth_attachment.image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);

		// Colour attachment
		create_image(device, 0,
					 VK_IMAGE_TYPE_2D,
					 VK_FORMAT_R8G8B8A8_SRGB,
					 extent,
					 1, 1,
					 VK_SAMPLE_COUNT_1_BIT,
					 VK_IMAGE_TILING_OPTIMAL,
					 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					 VK_SHARING_MODE_EXCLUSIVE,
					 VK_IMAGE_LAYOUT_UNDEFINED,
					 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					 offscreen_pass.colour_attachment.image,
					 offscreen_pass.colour_attachment.memory);

		// Colour attachment image view
		offscreen_pass.colour_attachment.image_view = create_image_view(device.logical_device, offscreen_pass.colour_attachment.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

		// Sampler for offscreen
		VkSamplerCreateInfo sampler_ci = vki::samplerCreateInfo(VK_FILTER_LINEAR,
																VK_FILTER_LINEAR,
																VK_SAMPLER_MIPMAP_MODE_LINEAR,
																VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																0.0f, 1.0f, 0.0f, 1.0f,
																VK_BORDER_COLOR_INT_OPAQUE_BLACK);

		VkResult sampler_create = vkCreateSampler(device.logical_device, &sampler_ci, nullptr, &offscreen_pass.sampler);
		if(sampler_create != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create offscreen sampler");
		}

		// Renderpass creation
		std::array<VkAttachmentDescription, 2> attachment_descriptions = {};
		attachment_descriptions[0]									   = {
			.format			= VK_FORMAT_R8G8B8A8_SRGB,
			.samples		= VK_SAMPLE_COUNT_1_BIT,
			.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp		= VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};
		attachment_descriptions[1] = {
			.format			= depth_format,
			.samples		= VK_SAMPLE_COUNT_1_BIT,
			.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference colour_ref		 = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
		VkAttachmentReference depth_ref			 = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
		VkSubpassDescription subpass_description = {
			.pipelineBindPoint		 = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount	 = 1,
			.pColorAttachments		 = &colour_ref,
			.pDepthStencilAttachment = &depth_ref,
		};

		std::array<VkSubpassDependency, 2> dependencies;
		dependencies[0] = {
			.srcSubpass		 = VK_SUBPASS_EXTERNAL,
			.dstSubpass		 = 0,
			.srcStageMask	 = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask	 = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask	 = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask	 = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		};
		dependencies[1] = {
			.srcSubpass		 = 0,
			.dstSubpass		 = VK_SUBPASS_EXTERNAL,
			.srcStageMask	 = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask	 = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask	 = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask	 = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		};

		// Renderpass creation
		VkRenderPassCreateInfo renderpass_ci = vki::renderPassCreateInfo();
		renderpass_ci.attachmentCount		 = attachment_descriptions.size();
		renderpass_ci.pAttachments			 = attachment_descriptions.data();
		renderpass_ci.subpassCount			 = 1;
		renderpass_ci.pSubpasses			 = &subpass_description;
		renderpass_ci.dependencyCount		 = dependencies.size();
		renderpass_ci.pDependencies			 = dependencies.data();

		VkResult renderpass_create = vkCreateRenderPass(device.logical_device, &renderpass_ci, nullptr, &offscreen_pass.renderpass);
		if(renderpass_create != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create offscreen's renderpass");
		}

		VkImageView attachments[2];
		attachments[0] = offscreen_pass.colour_attachment.image_view;
		attachments[1] = offscreen_pass.depth_attachment.image_view;

		VkFramebufferCreateInfo fbo_ci = vki::framebufferCreateInfo(offscreen_pass.renderpass, 2, attachments, swapchain.swapchain_extent.width, swapchain.swapchain_extent.height, 1);
		VkResult fbo_create			   = vkCreateFramebuffer(device.logical_device, &fbo_ci, nullptr, &offscreen_pass.framebuffer);
		if(fbo_create != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create offscreen framebuffer");
		}
	}


	void setup_descriptor_set_layout()
	{
		// ========================================================================
		//							SETUP FOR MODEL SHADER
		// ========================================================================

		VkDescriptorSetLayoutBinding ubo_layout_binding			= vki::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
		VkDescriptorSetLayoutBinding tex_sampler_layout_binding = vki::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

		// Put the descriptor set descriptions into a vector
		std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings_model;
		descriptor_set_layout_bindings_model.push_back(ubo_layout_binding);
		descriptor_set_layout_bindings_model.push_back(tex_sampler_layout_binding);

		VkDescriptorSetLayoutCreateInfo descriptor_set_ci = vki::descriptorSetLayoutCreateInfo(descriptor_set_layout_bindings_model.size(), descriptor_set_layout_bindings_model.data());
		VkResult descriptor_set_create					  = vkCreateDescriptorSetLayout(device.logical_device, &descriptor_set_ci, nullptr, &descriptor_set_layouts.model);
		if(descriptor_set_create != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create descriptor set layout");
		}

		// ========================================================================
		//							SETUP FOR FSQUAD SHADER
		// ========================================================================

		// Use the ubo_layout_binding and push it on as well
		VkDescriptorSetLayoutBinding server_framesampler_layout_binding	   = vki::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);
		VkDescriptorSetLayoutBinding local_rendered_sampler_layout_binding = vki::descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

		std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings_fsquad;
		descriptor_set_layout_bindings_fsquad.push_back(ubo_layout_binding);
		descriptor_set_layout_bindings_fsquad.push_back(server_framesampler_layout_binding);
		descriptor_set_layout_bindings_fsquad.push_back(local_rendered_sampler_layout_binding);

		descriptor_set_ci	  = vki::descriptorSetLayoutCreateInfo(descriptor_set_layout_bindings_fsquad.size(), descriptor_set_layout_bindings_fsquad.data());
		descriptor_set_create = vkCreateDescriptorSetLayout(device.logical_device, &descriptor_set_ci, nullptr, &descriptor_set_layouts.fsquad);
		if(descriptor_set_create != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create descriptor set layout");
		}
	}


	void setup_descriptor_pool()
	{
		VkDescriptorPoolSize poolsize_ubo	  = vki::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, swapchain.images.size());
		VkDescriptorPoolSize poolsize_sampler = vki::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * swapchain.images.size());
		printf("poolsize_sampler: %d\n", poolsize_sampler.descriptorCount);


		std::vector<VkDescriptorPoolSize> poolsizes;
		poolsizes.push_back(poolsize_ubo);
		poolsizes.push_back(poolsize_sampler);

		VkDescriptorPoolCreateInfo pool_ci = vki::descriptorPoolCreateInfo(2 * swapchain.images.size(), poolsizes.size(), poolsizes.data()); // two pools
		VkResult descriptor_pool_create	   = vkCreateDescriptorPool(device.logical_device, &pool_ci, nullptr, &descriptor_pool);
		if(descriptor_pool_create != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create descriptor pool");
		}
	}

	void setup_descriptor_sets()
	{
		// ========================================================================
		//							SETUP FOR MODEL SHADER
		// ========================================================================

		std::vector<VkDescriptorSetLayout> model_layouts(swapchain.images.size(), descriptor_set_layouts.model);
		descriptor_sets.model.resize(swapchain.images.size());

		// Allocate descriptor sets
		VkDescriptorSetAllocateInfo set_ai	 = vki::descriptorSetAllocateInfo(descriptor_pool, swapchain.images.size(), model_layouts.data());
		VkResult descriptor_set_alloc_result = vkAllocateDescriptorSets(device.logical_device, &set_ai, descriptor_sets.model.data());
		if(descriptor_set_alloc_result != VK_SUCCESS)
		{
			throw std::runtime_error("Could not allocate model descriptor set");
		}

		// Populate the descriptor sets
		for(uint32_t i = 0; i < swapchain.images.size(); i++)
		{
			VkDescriptorBufferInfo buffer_info	= vki::descriptorBufferInfo(ubos[i], 0, sizeof(UBO));
			VkDescriptorImageInfo teximage_info = vki::descriptorImageInfo(tex_sampler, texcolour_attachment.image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			std::vector<VkWriteDescriptorSet> write_descriptor_sets;
			write_descriptor_sets = {
				vki::writeDescriptorSet(descriptor_sets.model[i], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &buffer_info),
				vki::writeDescriptorSet(descriptor_sets.model[i], 1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &teximage_info),
			};

			vkUpdateDescriptorSets(device.logical_device, write_descriptor_sets.size(), write_descriptor_sets.data(), 0, nullptr);
		}

		// ========================================================================
		//							SETUP FOR FSQUAD SHADER
		// ========================================================================
		std::vector<VkDescriptorSetLayout> fsquad_layouts(swapchain.images.size(), descriptor_set_layouts.fsquad);
		descriptor_sets.fsquad.resize(swapchain.images.size());

		set_ai						= vki::descriptorSetAllocateInfo(descriptor_pool, swapchain.images.size(), fsquad_layouts.data());
		descriptor_set_alloc_result = vkAllocateDescriptorSets(device.logical_device, &set_ai, descriptor_sets.fsquad.data());
		if(descriptor_set_alloc_result != VK_SUCCESS)
		{
			throw std::runtime_error("Could not allocate fsquad descriptor set");
		}

		for(uint32_t i = 0; i < swapchain.images.size(); i++)
		{
			VkDescriptorBufferInfo buffer_info			   = vki::descriptorBufferInfo(ubos[i], 0, sizeof(UBO));
			VkDescriptorImageInfo serverimage_info		   = vki::descriptorImageInfo(server_frame_sampler, server_colour_attachment.image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			VkDescriptorImageInfo local_renderedimage_info = vki::descriptorImageInfo(offscreen_pass.sampler, offscreen_pass.colour_attachment.image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			std::vector<VkWriteDescriptorSet> write_descriptor_sets;
			write_descriptor_sets = {
				vki::writeDescriptorSet(descriptor_sets.fsquad[i], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &buffer_info),
				vki::writeDescriptorSet(descriptor_sets.fsquad[i], 1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &serverimage_info),
				vki::writeDescriptorSet(descriptor_sets.fsquad[i], 2, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &local_renderedimage_info),
			};

			vkUpdateDescriptorSets(device.logical_device, write_descriptor_sets.size(), write_descriptor_sets.data(), 0, nullptr);
		}
	}

	void setup_depth()
	{
	}



	void setup_texture_sampler()
	{
		// Load the image
		int texture_width;
		int texture_height;
		int texture_channels;

		stbi_uc *pixels			  = stbi_load(TEXTURE_PATH.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
		VkDeviceSize texture_size = texture_width * texture_height * 4;

		if(!pixels)
		{
			throw std::runtime_error("Could not load texture image");
		}

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;

		create_buffer(device, texture_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
		void *data;
		vkMapMemory(device.logical_device, staging_buffer_memory, 0, texture_size, 0, &data);
		memcpy(data, pixels, texture_size);
		vkUnmapMemory(device.logical_device, staging_buffer_memory);

		stbi_image_free(pixels);


		VkExtent3D texextent3D = {
			.width	= (uint32_t) texture_width,
			.height = (uint32_t) texture_height,
			.depth	= 1,
		};


		create_image(device, 0,
					 VK_IMAGE_TYPE_2D,
					 VK_FORMAT_R8G8B8A8_SRGB,
					 texextent3D,
					 1, 1,
					 VK_SAMPLE_COUNT_1_BIT,
					 VK_IMAGE_TILING_OPTIMAL,
					 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					 VK_SHARING_MODE_EXCLUSIVE,
					 VK_IMAGE_LAYOUT_UNDEFINED,
					 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					 texcolour_attachment.image,
					 texcolour_attachment.memory);
		transition_image_layout(device, command_pool, texcolour_attachment.image,
								VK_FORMAT_R8G8B8A8_SRGB,
								VK_IMAGE_LAYOUT_UNDEFINED,
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copy_buffer_to_image(device, command_pool, staging_buffer,
							 texcolour_attachment.image,
							 texture_width,
							 texture_height);
		transition_image_layout(device, command_pool, texcolour_attachment.image,
								VK_FORMAT_R8G8B8A8_SRGB,
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device.logical_device, staging_buffer, nullptr);
		vkFreeMemory(device.logical_device, staging_buffer_memory, nullptr);

		texcolour_attachment.image_view = create_image_view(device.logical_device, texcolour_attachment.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, true);
		std::cout << "texcolour imageview: " << texcolour_attachment.image_view << std::endl;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device.physical_device, &properties);
		VkSamplerCreateInfo sampler_ci = vki::samplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
																VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
																0.0f, VK_TRUE, properties.limits.maxSamplerAnisotropy, VK_FALSE, VK_COMPARE_OP_ALWAYS,
																0.0f, 0.0f, VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_FALSE);

		VkResult sampler_create = vkCreateSampler(device.logical_device, &sampler_ci, nullptr, &tex_sampler);
		if(sampler_create != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create texture sampler");
		}
	}


	void setup_serverframe_sampler()
	{
		VkExtent3D texextent3D = {
			.width	= (uint32_t) SERVERWIDTH,
			.height = (uint32_t) SERVERHEIGHT,
			.depth	= 1,
		};

		// Create image that will be bound to sampler
		create_image(device, 0,
					 VK_IMAGE_TYPE_2D,
					 VK_FORMAT_R8G8B8A8_SRGB,
					 texextent3D,
					 1, 1,
					 VK_SAMPLE_COUNT_1_BIT,
					 VK_IMAGE_TILING_OPTIMAL,
					 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					 VK_SHARING_MODE_EXCLUSIVE,
					 VK_IMAGE_LAYOUT_UNDEFINED,
					 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					 server_colour_attachment.image,
					 server_colour_attachment.memory);

		// Transition it to transfer dst optimal since it can't be directly transitioned to shader read-only optimal
		transition_image_layout(device, command_pool,
								server_colour_attachment.image,
								VK_FORMAT_R8G8B8A8_SRGB,
								VK_IMAGE_LAYOUT_UNDEFINED,
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Now transition to shader read only optimal
		transition_image_layout(device, command_pool,
								server_colour_attachment.image,
								VK_FORMAT_R8G8B8A8_SRGB,
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// Create image view for the colour attachment
		server_colour_attachment.image_view = create_image_view(device.logical_device, server_colour_attachment.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

		// Create the sampler
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device.physical_device, &properties);
		VkSamplerCreateInfo sampler_ci = vki::samplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
																VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
																0.0f, VK_TRUE, properties.limits.maxSamplerAnisotropy, VK_FALSE, VK_COMPARE_OP_ALWAYS,
																0.0f, 0.0f, VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_FALSE);

		VkResult sampler_create = vkCreateSampler(device.logical_device, &sampler_ci, nullptr, &server_frame_sampler);
		if(sampler_create != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create texture sampler");
		}
	}


	void initialize_ubos()
	{
		VkDeviceSize buffersize = sizeof(UBO);
		ubos.resize(swapchain.images.size());
		ubos_mem.resize(swapchain.images.size());

		for(uint32_t i = 0; i < swapchain.images.size(); i++)
		{
			create_buffer(device, buffersize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ubos[i], ubos_mem[i]);
		}
	}


	// Provide a new transformation every frame
	void update_ubos(uint32_t current_image_index)
	{
		static std::chrono::_V2::system_clock::time_point start_time = std::chrono::high_resolution_clock::now();
		std::chrono::_V2::system_clock::time_point current_time		 = std::chrono::high_resolution_clock::now();
		float dt													 = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

		camera.move(dt, window);

		UBOClient ubo = {
			.model		= glm::mat4(1.0f),
			.view		= glm::lookAt(camera.position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
			.projection = glm::perspective(glm::radians(45.0f), swapchain.swapchain_extent.width / (float) swapchain.swapchain_extent.height, 0.1f, 10.0f),
		};
		ubo.projection[1][1] *= -1; // flip y coordinate from opengl

		void *data;
		vkMapMemory(device.logical_device, ubos_mem[current_image_index], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device.logical_device, ubos_mem[current_image_index]);
	}


	void setup_graphics_pipeline()
	{
		// ========================================================================
		//							SETUP FOR MODEL SHADER
		// ========================================================================

		std::vector<char> vertex_shader_code   = parse_shader_file("shaders/vertexmodelclient.spv");
		std::vector<char> fragment_shader_code = parse_shader_file("shaders/fragmentmodelclient.spv");
		VkShaderModule vertex_shader_module	   = setup_shader_module(vertex_shader_code, device);
		VkShaderModule fragment_shader_module  = setup_shader_module(fragment_shader_code, device);


		// Vertex binding setup
		VkVertexInputBindingDescription binding_desc				  = Vertex::get_binding_description();
		std::vector<VkVertexInputAttributeDescription> attribute_desc = Vertex::get_attribute_descriptions();

		VkPipelineShaderStageCreateInfo vertex_shader_stage_info   = vki::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_module, "main");
		VkPipelineShaderStageCreateInfo fragment_shader_stage_info = vki::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_module, "main");

		VkPipelineShaderStageCreateInfo shader_stages[] = {vertex_shader_stage_info, fragment_shader_stage_info};

		VkPipelineVertexInputStateCreateInfo vertex_input_info = vki::pipelineVertexInputStateCreateInfo(1, &binding_desc, attribute_desc.size(), attribute_desc.data());
		VkPipelineInputAssemblyStateCreateInfo input_assembly  = vki::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

		// TODO: Might need to think about where to have the quad's viewport
		VkViewport viewport								 = vki::viewport(0.0f, 0.0f, (float) swapchain.swapchain_extent.width, (float) swapchain.swapchain_extent.height, 0.0f, 1.0f);
		VkRect2D scissor								 = vki::rect2D({0, 0}, swapchain.swapchain_extent);
		VkPipelineViewportStateCreateInfo viewport_state = vki::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

		VkPipelineRasterizationStateCreateInfo rasterizer			= vki::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		VkPipelineMultisampleStateCreateInfo multisampling			= vki::pipelineMultisampleStateCreateInfo(VK_FALSE, VK_SAMPLE_COUNT_1_BIT);
		VkPipelineColorBlendAttachmentState colour_blend_attachment = vki::pipelineColorBlendAttachmentState(VK_FALSE, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
		VkPipelineColorBlendStateCreateInfo colour_blending			= vki::pipelineColorBlendStateCreateInfo(1, &colour_blend_attachment);
		VkPipelineDepthStencilStateCreateInfo depth_stencil			= vki::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE, {}, {}, 0.0f, 1.0f);


		VkPipelineLayoutCreateInfo pipeline_layout_info_model = vki::pipelineLayoutCreateInfo(1, &descriptor_set_layouts.model, 0, nullptr);

		if(vkCreatePipelineLayout(device.logical_device, &pipeline_layout_info_model, nullptr, &pipeline_layouts.model) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipeline_ci = vki::graphicsPipelineCreateInfo();
		pipeline_ci.stageCount					 = 2;
		pipeline_ci.pStages						 = shader_stages;
		pipeline_ci.pVertexInputState			 = &vertex_input_info;
		pipeline_ci.pInputAssemblyState			 = &input_assembly;
		pipeline_ci.pViewportState				 = &viewport_state;
		pipeline_ci.pRasterizationState			 = &rasterizer;
		pipeline_ci.pMultisampleState			 = &multisampling;
		pipeline_ci.pColorBlendState			 = &colour_blending;
		pipeline_ci.pDepthStencilState			 = &depth_stencil;
		pipeline_ci.layout						 = pipeline_layouts.model;
		pipeline_ci.renderPass					 = offscreen_pass.renderpass;
		pipeline_ci.subpass						 = 0;
		pipeline_ci.basePipelineHandle			 = VK_NULL_HANDLE;

		if(vkCreateGraphicsPipelines(device.logical_device, VK_NULL_HANDLE, 1, &pipeline_ci, nullptr, &pipelines.model) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create graphics pipeline!");
		}


		// ========================================================================
		//							SETUP FOR FSQUAD SHADER
		// ========================================================================

		vertex_shader_code		   = parse_shader_file("shaders/vertexfsquadclient.spv");
		fragment_shader_code	   = parse_shader_file("shaders/fragmentfsquadclient.spv");
		vertex_shader_module	   = setup_shader_module(vertex_shader_code, device);
		fragment_shader_module	   = setup_shader_module(fragment_shader_code, device);
		vertex_shader_stage_info   = vki::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_module, "main");
		fragment_shader_stage_info = vki::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader_module, "main");
		shader_stages[0]		   = vertex_shader_stage_info;
		shader_stages[1]		   = fragment_shader_stage_info;

		VkPipelineVertexInputStateCreateInfo empty_vertex_input_info = {};
		empty_vertex_input_info.sType								 = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		// Cull front bit for FS quad
		rasterizer											   = vki::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		VkPipelineLayoutCreateInfo pipeline_layout_info_fsquad = vki::pipelineLayoutCreateInfo(1, &descriptor_set_layouts.fsquad, 0, nullptr);

		if(vkCreatePipelineLayout(device.logical_device, &pipeline_layout_info_fsquad, nullptr, &pipeline_layouts.fsquad) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}

		// Modify the current graphics pipeline ci
		pipeline_ci.pVertexInputState = &empty_vertex_input_info;
		pipeline_ci.layout			  = pipeline_layouts.fsquad;
		pipeline_ci.renderPass		  = renderpass.renderpass;

		if(vkCreateGraphicsPipelines(device.logical_device, VK_NULL_HANDLE, 1, &pipeline_ci, nullptr, &pipelines.fsquad) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(device.logical_device, fragment_shader_module, nullptr);
		vkDestroyShaderModule(device.logical_device, vertex_shader_module, nullptr);
	}

	void setup_framebuffers()
	{
		// ========================================================================
		//							SETUP FOR FSQUAD SHADER
		// ========================================================================

		swapchain.framebuffers.resize(swapchain.image_views.size());

		for(size_t i = 0; i < swapchain.image_views.size(); i++)
		{
			std::vector<VkImageView> attachments = {swapchain.image_views[i], offscreen_pass.depth_attachment.image_view};
			VkFramebufferCreateInfo fbo_ci		 = vki::framebufferCreateInfo(renderpass.renderpass, attachments.size(), attachments.data(), swapchain.swapchain_extent.width, swapchain.swapchain_extent.height, 1);

			if(vkCreateFramebuffer(device.logical_device, &fbo_ci, nullptr, &swapchain.framebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create swapchain framebuffer!");
			}
		}

		// ========================================================================
		//							SETUP FOR MODEL SHADER
		// ========================================================================
		// this was done in setup_offscreen(), maybe I'll move it back here someday!!
	}


	void setup_command_pool()
	{
		QueueFamilyIndices qf_indices = search_queue_families(device.physical_device, surface);

		VkCommandPoolCreateInfo pool_ci = vki::commandPoolCreateInfo(qf_indices.graphics_qf);
		if(vkCreateCommandPool(device.logical_device, &pool_ci, nullptr, &command_pool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command pool!");
		}
	}

	struct FirstRenderPassArgs
	{
		OffscreenPass offscreen_pass;
		VulkanSwapchain swapchain;
		Pipelines pipelines;
		VkCommandBuffer cmdbuf;
		VkDescriptorSet descriptor_set;
		VkBuffer vbo;
		VkBuffer ibo;
		PipelineLayouts pipeline_layouts;
		Model model;
	};

	static void *execute_first_renderpass(void *renderpassargs)
	{
		FirstRenderPassArgs *args = (FirstRenderPassArgs *) renderpassargs;

		// First pass: The offscreen rendering
		{
			VkClearValue clear_values[2];
			clear_values[0].color				= {0.0f, 0.0f, 0.0f, 1.0f};
			clear_values[1].depthStencil		= {1.0f, 0};
			VkRenderPassBeginInfo renderpass_bi = vki::renderPassBeginInfo(args->offscreen_pass.renderpass,
																		   args->offscreen_pass.framebuffer,
																		   {0, 0},
																		   args->swapchain.swapchain_extent,
																		   2, clear_values);

			vkCmdBeginRenderPass(args->cmdbuf, &renderpass_bi, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(args->cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, args->pipelines.model);

			VkBuffer vertex_buffers[] = {args->vbo};
			VkDeviceSize offsets[]	  = {0};

			vkCmdBindVertexBuffers(args->cmdbuf, 0, 1, vertex_buffers, offsets);
			vkCmdBindIndexBuffer(args->cmdbuf, args->ibo, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(args->cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, args->pipeline_layouts.model, 0, 1, &args->descriptor_set, 0, nullptr);

			vkCmdDrawIndexed(args->cmdbuf, args->model.indices.size(), 1, 0, 0, 0);

			vkCmdEndRenderPass(args->cmdbuf);
		}

		return nullptr;
	}

	void setup_command_buffers()
	{
		command_buffers.resize(swapchain.framebuffers.size());

		VkCommandBufferAllocateInfo cmdbuf_ai = vki::commandBufferAllocateInfo(nullptr, command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, command_buffers.size());

		if(vkAllocateCommandBuffers(device.logical_device, &cmdbuf_ai, command_buffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate command buffers!");
		}

		for(size_t i = 0; i < command_buffers.size(); i++)
		{
			VkCommandBufferBeginInfo cmdbuf_bi = vki::commandBufferBeginInfo();

			if(vkBeginCommandBuffer(command_buffers[i], &cmdbuf_bi) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			FirstRenderPassArgs renderpassargs = {offscreen_pass, swapchain, pipelines, command_buffers[i], descriptor_sets.model[i], vbo, ibo, pipeline_layouts, model};
			int first_renderpass_thread		   = pthread_create(&vk_pthread_t.first_renderpass_thread, nullptr, DeviceRenderer::execute_first_renderpass, (void *) &renderpassargs);

			if(i == 0)
			{
				pthread_join(vk_pthread_t.rec_image_thread, nullptr);
			}
			pthread_join(vk_pthread_t.first_renderpass_thread, nullptr);

			// Second renderpass: Fullscreen quad draw
			{
				VkClearValue clear_values[2];
				clear_values[0].color		 = {0.0f, 0.0f, 0.0f, 1.0f};
				clear_values[1].depthStencil = {1.0f, 0};

				VkRenderPassBeginInfo renderpass_bi = vki::renderPassBeginInfo(renderpass.renderpass,
																			   swapchain.framebuffers[i],
																			   {0, 0},
																			   swapchain.swapchain_extent,
																			   2, clear_values);

				vkCmdBeginRenderPass(command_buffers[i], &renderpass_bi, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.fsquad);
				vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.fsquad, 0, 1, &descriptor_sets.fsquad[i], 0, nullptr);
				vkCmdDraw(command_buffers[i], 3, 1, 0, 0);
				vkCmdEndRenderPass(command_buffers[i]);
			}


			if(vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}


	void setup_vk_async()
	{
		image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
		images_in_flight.resize(swapchain.images.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphore_ci = vki::semaphoreCreateInfo();
		VkFenceCreateInfo fence_ci		   = vki::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if(vkCreateSemaphore(device.logical_device, &semaphore_ci, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
			   vkCreateSemaphore(device.logical_device, &semaphore_ci, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
			   vkCreateFence(device.logical_device, &fence_ci, nullptr, &in_flight_fences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}


	void render_complete_frame()
	{

		timeval timer_start;
		timeval timer_end;
		gettimeofday(&timer_start, nullptr);

		// Write camera data (position, front) to server
		float camera_data[6] = {
			camera.position.x,
			camera.position.y,
			camera.position.z,

			camera.front.x,
			camera.front.y,
			camera.front.z,
		};
		//write(client.socket_fd, camera_data, 6 * sizeof(float));

		vkWaitForFences(device.logical_device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

		// Make a thread for the swapchain image
		int receive_image_thread_create = pthread_create(&vk_pthread_t.rec_image_thread, nullptr, DeviceRenderer::receive_swapchain_image, this);
		setup_command_buffers();
		//pthread_join(vk_pthread_t.rec_image_thread, nullptr);

		uint32_t image_index;
		VkResult result = vkAcquireNextImageKHR(device.logical_device, swapchain.swapchain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

		// Check that the swapchain is incompatible with the surface (window resizing)
		if(result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			swapchain_recreation();
			return;
		}

		update_ubos(image_index);

		if(images_in_flight[image_index] != VK_NULL_HANDLE)
		{
			vkWaitForFences(device.logical_device, 1, &images_in_flight[image_index], VK_TRUE, UINT64_MAX);
		}
		images_in_flight[image_index] = in_flight_fences[current_frame];

		VkSemaphore wait_semaphores[]	   = {image_available_semaphores[current_frame]};
		VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		VkSemaphore signal_semaphores[]	   = {render_finished_semaphores[current_frame]};

		VkSubmitInfo submit_info		 = vki::submitInfo();
		submit_info.waitSemaphoreCount	 = 1;
		submit_info.pWaitSemaphores		 = wait_semaphores;
		submit_info.pWaitDstStageMask	 = wait_stages;
		submit_info.commandBufferCount	 = 1;
		submit_info.pCommandBuffers		 = &command_buffers[image_index];
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores	 = signal_semaphores;

		vkResetFences(device.logical_device, 1, &in_flight_fences[current_frame]);

		if(vkQueueSubmit(device.graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}


		VkSwapchainKHR swapchains_to_present_to[] = {swapchain.swapchain};
		VkPresentInfoKHR present_info			  = vki::presentInfoKHR(1, signal_semaphores, 1, swapchains_to_present_to, &image_index);

		vkQueuePresentKHR(device.present_queue, &present_info);


		current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

		gettimeofday(&timer_end, nullptr);


		double dt = timer_end.tv_sec - timer_start.tv_sec + (timer_end.tv_usec - timer_start.tv_usec);
		printf("numframe: %lu\tframe dt: %f\n", numframes, (dt / 1000000.0f));

		numframes++;
	}

	// Test function adapted from sasha's example screenshot
	static void *receive_swapchain_image(void *devicerenderer)
	{
		DeviceRenderer *dr = (DeviceRenderer *) devicerenderer;

		char *data;
		VkDeviceSize memcpy_offset = 0;
		std::string filename	   = "tmpclient" + std::to_string(dr->numframes) + ".ppm";

		/*std::ofstream file(filename, std::ios::out | std::ios::binary);
		file << "P6\n"
			 << SERVERWIDTH << "\n"
			 << SERVERHEIGHT << "\n"
			 << 255 << "\n";*/

		// Create buffer to read from tcp socket
		uint32_t servbuf[SERVERWIDTH * SERVERHEIGHT];
		VkDeviceSize num_bytes = SERVERWIDTH * SERVERHEIGHT / 4 * sizeof(uint32_t);

		// Receive & map memory
		VkDeviceSize memmap_offset = 0;

		// Map in batches of 128 rows
		for(uint16_t i = 0; i < 4; i++)
		{
			size_t servbufidx = 0;
			do
			{
				ssize_t server_read = read(dr->client.socket_fd, &servbuf[servbufidx], num_bytes - servbufidx);
				servbufidx += server_read;
			} while(servbufidx < num_bytes);

			printf("Read %d\t%zu\n", i, servbufidx);

			vkMapMemory(dr->device.logical_device, dr->image_buffer_memory, memmap_offset, num_bytes, 0, (void **) &data);
			memcpy(data, servbuf, (size_t) num_bytes);
			vkUnmapMemory(dr->device.logical_device, dr->image_buffer_memory);
			memmap_offset += num_bytes;

			// Transmit to server that code was written
			char end_line_code[1] = {'d'};
			write(dr->client.socket_fd, end_line_code, 1);
		}

		// Now the VkBuffer should be filled with memory that we can copy to a swapchain image.
		// Transition swapchain image to copyable layout
		VkCommandBuffer copy_cmdbuf = begin_command_buffer(dr->device, dr->command_pool);

		// Transition current swapchain image to be transfer_dst_optimal. Need to note the src and dst access masks
		transition_image_layout(dr->device, dr->command_pool, copy_cmdbuf,
								dr->server_colour_attachment.image,
								VK_ACCESS_MEMORY_READ_BIT,				  // src access_mask
								VK_ACCESS_TRANSFER_WRITE_BIT,			  // dst access_mask
								VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // current layout
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,	  // new layout to transfer to (destination)
								VK_PIPELINE_STAGE_TRANSFER_BIT,			  // dst pipeline mask
								VK_PIPELINE_STAGE_TRANSFER_BIT);		  // src pipeline mask

		// Image subresource to be used in the vkbufferimagecopy
		VkImageSubresourceLayers image_subresource = {
			.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
			.baseArrayLayer = 0,
			.layerCount		= 1,
		};

		// Create the vkbufferimagecopy pregions
		VkBufferImageCopy copy_region = {
			.bufferOffset	   = 0,
			.bufferRowLength   = SERVERWIDTH,
			.bufferImageHeight = SERVERHEIGHT,
			.imageSubresource  = image_subresource,
			.imageExtent	   = {SERVERWIDTH, SERVERHEIGHT, 1},
		};


		// Perform the copy
		vkCmdCopyBufferToImage(copy_cmdbuf,
							   dr->image_buffer,
							   dr->server_colour_attachment.image,
							   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							   1, &copy_region);

		// Transition swapchain image back
		transition_image_layout(dr->device, dr->command_pool, copy_cmdbuf,
								dr->server_colour_attachment.image,
								VK_ACCESS_TRANSFER_WRITE_BIT,			  // src access mask
								VK_ACCESS_MEMORY_READ_BIT,				  // dst access mask
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,	  // current layout
								VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // layout transitioning to
								VK_PIPELINE_STAGE_TRANSFER_BIT,			  // pipeline flags
								VK_PIPELINE_STAGE_TRANSFER_BIT);		  // pipeline flags

		end_command_buffer(dr->device, dr->command_pool, copy_cmdbuf);

		return nullptr;
	}


	void create_copy_image_buffer()
	{
		// Create a VkBuffer
		VkDeviceSize image_buffer_size = CLIENTWIDTH * CLIENTHEIGHT * sizeof(uint32_t);
		create_buffer(device, image_buffer_size,
					  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					  image_buffer, image_buffer_memory);
	}


	void swapchain_recreation()
	{
		vkDeviceWaitIdle(device.logical_device);

		SwapChainSupportDetails swapchain_support = query_swapchain_support(device.physical_device, surface);
		swapchain.setup_swapchain(swapchain_support, surface, device, window);
		swapchain.setup_image_views(device.logical_device);
		renderpass.setup_renderpass(device, swapchain);
		setup_graphics_pipeline();
		setup_framebuffers();
		initialize_ubos();
		setup_descriptor_pool();
		setup_command_buffers();
	}
};





int main()
{
	DeviceRenderer device_renderer;
	device_renderer.run();

	return 0;
}