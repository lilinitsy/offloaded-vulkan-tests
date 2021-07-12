#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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

#define GLM_FORCE_RADIANS
#define GLMFORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>

#include "Vertex.h"
#include "defines.h"
#include "utils.h"
#include "vk_debug_messenger.h"
#include "vk_device.h"
#include "vk_image.h"
#include "vk_models.h"
#include "vk_queuefamilies.h"
#include "vk_renderpass.h"
#include "vk_shaders.h"
#include "vk_swapchain.h"
#include "vk_swapchain_support.h"

std::string MODEL_PATH	 = "../models/laurenscan/Model.obj";
std::string TEXTURE_PATH = "../models/laurenscan/Model.jpg";


#define PORT 1234


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
			.sin_port	= static_cast<in_port_t>(port),
		};


		int connect_result = connect(socket_fd, (sockaddr *) &server_address, sizeof(server_address));
		if(connect_result == -1)
		{
			throw std::runtime_error("Could not connect to server");
		}
	}

	void run()
	{
		uint32_t servbuf[1920 * 3];

		std::ofstream file("tmp.ppm", std::ios::out | std::ios::binary);
		file << "P6\n"
			 << 1920 << "\n"
			 << 1080 << "\n"
			 << 255 << "\n";

		int height = 0;

		while(1)
		{
			int server_read = read(socket_fd, servbuf, 1920 * 3);

			if(server_read != -1)
			{
				uint32_t *row = servbuf;
				char *rowchar = (char *) row;

				for(uint8_t i = 0; i < 3; i++)
				{
					printf("row: %c\n", rowchar[i]);
				}

				for(uint32_t x = 0; x < 1920; x++)
				{
					//file.write((char *) row, 3);
					row++;
				}

				height++;

				write(socket_fd, "linedone", 8);
			}

			else
			{
				printf("Nothing to read\n");
			}

			if(height == 1920)
			{
				//file.close();
				printf("height = 1080\n");
				height = 0;
			}
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

	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;

	Model model;
	VkBuffer vbo;
	VkBuffer ibo;
	std::vector<VkBuffer> ubos;
	std::vector<VkDeviceMemory> ubos_mem;
	VkDeviceMemory vbo_mem;
	VkDeviceMemory ibo_mem;

	VulkanAttachment colour_attachment;
	VkSampler tex_sampler;
	VulkanAttachment depth_attachment;

	// Buffer that will be copied to
	VkBuffer image_buffer;
	VkDeviceMemory image_buffer_memory;

	VkCommandPool command_pool;
	std::vector<VkCommandBuffer> command_buffers;

	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool;
	std::vector<VkDescriptorSet> descriptor_sets;

	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;
	std::vector<VkFence> images_in_flight;
	uint32_t current_frame = 0;

	Client client;

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(CLIENTWIDTH, CLIENTHEIGHT, "Vulkan", nullptr, nullptr);
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
		setup_descriptor_set_layout();
		setup_graphics_pipeline();
		setup_command_pool();
		setup_depth();
		setup_framebuffers();
		setup_texture();
		setup_texture_image();
		setup_sampler();
		model = Model(MODEL_PATH, TEXTURE_PATH, glm::vec3(-1.0f, -0.5f, 0.5f));
		initialize_vertex_buffers(device, model.vertices, &vbo, &vbo_mem, command_pool);
		initialize_index_buffers(device, model.indices, &ibo, &ibo_mem, command_pool);
		initialize_ubos();
		setup_descriptor_pool();
		setup_descriptor_sets();
		setup_command_buffers();
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

		vkDestroyImageView(device.logical_device, colour_attachment.image_view, nullptr);
		vkDestroySampler(device.logical_device, tex_sampler, nullptr);
		vkDestroyImage(device.logical_device, colour_attachment.image, nullptr);
		vkFreeMemory(device.logical_device, colour_attachment.memory, nullptr);

		vkDestroyDescriptorSetLayout(device.logical_device, descriptor_set_layout, nullptr);

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
		vkDestroyPipeline(device.logical_device, graphics_pipeline, nullptr);
		vkDestroyPipelineLayout(device.logical_device, pipeline_layout, nullptr);
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


	void setup_descriptor_set_layout()
	{
		VkDescriptorSetLayoutBinding ubo_layout_binding		= vki::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
		VkDescriptorSetLayoutBinding sampler_layout_binding = vki::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

		// Put the descriptor set descriptions into a vector
		std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;
		descriptor_set_layout_bindings.push_back(ubo_layout_binding);
		descriptor_set_layout_bindings.push_back(sampler_layout_binding);

		VkDescriptorSetLayoutCreateInfo descriptor_set_ci = vki::descriptorSetLayoutCreateInfo(descriptor_set_layout_bindings.size(), descriptor_set_layout_bindings.data());
		VkResult descriptor_set_create					  = vkCreateDescriptorSetLayout(device.logical_device, &descriptor_set_ci, nullptr, &descriptor_set_layout);
		if(descriptor_set_create != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create descriptor set layout");
		}
	}

	void setup_descriptor_pool()
	{
		VkDescriptorPoolSize poolsize_ubo	  = vki::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, swapchain.images.size());
		VkDescriptorPoolSize poolsize_sampler = vki::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, swapchain.images.size());

		std::vector<VkDescriptorPoolSize> poolsizes;
		poolsizes.push_back(poolsize_ubo);
		poolsizes.push_back(poolsize_sampler);

		VkDescriptorPoolCreateInfo pool_ci = vki::descriptorPoolCreateInfo(swapchain.images.size(), poolsizes.size(), poolsizes.data()); // two pools
		VkResult descriptor_pool_create	   = vkCreateDescriptorPool(device.logical_device, &pool_ci, nullptr, &descriptor_pool);
		if(descriptor_pool_create != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create descriptor pool");
		}
	}

	void setup_descriptor_sets()
	{
		std::vector<VkDescriptorSetLayout> layouts(swapchain.images.size(), descriptor_set_layout);
		descriptor_sets.resize(swapchain.images.size());

		// Allocate descriptor sets
		VkDescriptorSetAllocateInfo set_ai	 = vki::descriptorSetAllocateInfo(descriptor_pool, swapchain.images.size(), layouts.data());
		VkResult descriptor_set_alloc_result = vkAllocateDescriptorSets(device.logical_device, &set_ai, descriptor_sets.data());
		if(descriptor_set_alloc_result != VK_SUCCESS)
		{
			throw std::runtime_error("Could not allocate descriptor set");
		}

		// Populate the descriptor sets
		for(uint32_t i = 0; i < swapchain.images.size(); i++)
		{
			VkDescriptorBufferInfo buffer_info = vki::descriptorBufferInfo(ubos[i], 0, sizeof(UBO));
			VkDescriptorImageInfo image_info   = vki::descriptorImageInfo(tex_sampler, colour_attachment.image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			std::vector<VkWriteDescriptorSet> write_descriptor_sets;
			write_descriptor_sets = {
				vki::writeDescriptorSet(descriptor_sets[i], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &buffer_info),
				vki::writeDescriptorSet(descriptor_sets[i], 1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_info),
			};

			vkUpdateDescriptorSets(device.logical_device, write_descriptor_sets.size(), write_descriptor_sets.data(), 0, nullptr);
		}
	}


	void setup_depth()
	{
		std::vector<VkFormat> formats = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
		VkFormat depth_format		  = device.find_format(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		VkExtent3D extent			  = {swapchain.swapchain_extent.width, swapchain.swapchain_extent.height, 1};

		create_image(device, 0, VK_IMAGE_TYPE_2D, depth_format, extent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_IMAGE_LAYOUT_UNDEFINED, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_attachment.image, depth_attachment.memory);
		depth_attachment.image_view = create_image_view(device.logical_device, depth_attachment.image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
	}


	void setup_texture()
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


		create_image(device, 0, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, texextent3D, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_IMAGE_LAYOUT_UNDEFINED, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colour_attachment.image, colour_attachment.memory);
		transition_image_layout(device, command_pool, colour_attachment.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copy_buffer_to_image(device, command_pool, staging_buffer, colour_attachment.image, texture_width, texture_height);
		transition_image_layout(device, command_pool, colour_attachment.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device.logical_device, staging_buffer, nullptr);
		vkFreeMemory(device.logical_device, staging_buffer_memory, nullptr);
	}

	void setup_texture_image()
	{
		colour_attachment.image_view = create_image_view(device.logical_device, colour_attachment.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void setup_sampler()
	{
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

		UBO ubo = {
			.model		= glm::rotate(glm::mat4(1.0f), dt * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			.view		= glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
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
		std::vector<char> vertex_shader_code   = parse_shader_file("shaders/vertexdefault.spv");
		std::vector<char> fragment_shader_code = parse_shader_file("shaders/fragmentdefault.spv");
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

		VkViewport viewport								 = vki::viewport(0.0f, 0.0f, (float) swapchain.swapchain_extent.width, (float) swapchain.swapchain_extent.height, 0.0f, 1.0f);
		VkRect2D scissor								 = vki::rect2D({0, 0}, swapchain.swapchain_extent);
		VkPipelineViewportStateCreateInfo viewport_state = vki::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

		VkPipelineRasterizationStateCreateInfo rasterizer			= vki::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		VkPipelineMultisampleStateCreateInfo multisampling			= vki::pipelineMultisampleStateCreateInfo(VK_FALSE, VK_SAMPLE_COUNT_1_BIT);
		VkPipelineColorBlendAttachmentState colour_blend_attachment = vki::pipelineColorBlendAttachmentState(VK_FALSE, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
		VkPipelineColorBlendStateCreateInfo colour_blending			= vki::pipelineColorBlendStateCreateInfo(1, &colour_blend_attachment);
		VkPipelineDepthStencilStateCreateInfo depth_stencil			= vki::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE, {}, {}, 0.0f, 1.0f);


		VkPipelineLayoutCreateInfo pipeline_layout_info = vki::pipelineLayoutCreateInfo(1, &descriptor_set_layout, 0, nullptr);

		if(vkCreatePipelineLayout(device.logical_device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
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
		pipeline_ci.layout						 = pipeline_layout;
		pipeline_ci.renderPass					 = renderpass.renderpass;
		pipeline_ci.subpass						 = 0;
		pipeline_ci.basePipelineHandle			 = VK_NULL_HANDLE;

		if(vkCreateGraphicsPipelines(device.logical_device, VK_NULL_HANDLE, 1, &pipeline_ci, nullptr, &graphics_pipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(device.logical_device, fragment_shader_module, nullptr);
		vkDestroyShaderModule(device.logical_device, vertex_shader_module, nullptr);
	}

	void setup_framebuffers()
	{
		swapchain.framebuffers.resize(swapchain.image_views.size());

		for(size_t i = 0; i < swapchain.image_views.size(); i++)
		{
			std::vector<VkImageView> attachments = {swapchain.image_views[i], depth_attachment.image_view};
			VkFramebufferCreateInfo fbo_ci		 = vki::framebufferCreateInfo(renderpass.renderpass, attachments.size(), attachments.data(), swapchain.swapchain_extent.width, swapchain.swapchain_extent.height, 1);

			if(vkCreateFramebuffer(device.logical_device, &fbo_ci, nullptr, &swapchain.framebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
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

			VkClearValue clear_values[2];
			clear_values[0].color				= {0.0f, 0.0f, 0.0f, 1.0f};
			clear_values[1].depthStencil		= {1.0f, 0};
			VkRenderPassBeginInfo renderpass_bi = vki::renderPassBeginInfo(renderpass.renderpass, swapchain.framebuffers[i], {0, 0}, swapchain.swapchain_extent, 2, clear_values);

			vkCmdBeginRenderPass(command_buffers[i], &renderpass_bi, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

			VkBuffer vertex_buffers[] = {vbo};
			VkDeviceSize offsets[]	  = {0};

			//vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);
			//vkCmdBindIndexBuffer(command_buffers[i], ibo, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[i], 0, nullptr);

			//vkCmdDrawIndexed(command_buffers[i], model.indices.size(), 1, 0, 0, 0);

			vkCmdEndRenderPass(command_buffers[i]);

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

		vkWaitForFences(device.logical_device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

		uint32_t image_index;
		VkResult result = vkAcquireNextImageKHR(device.logical_device, swapchain.swapchain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

		// Check that the swapchain is incompatible with the surface (window resizing)
		if(result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			swapchain_recreation();
			return;
		}

		//update_ubos(image_index);

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

		receive_swapchain_image(image_index);
		vkQueuePresentKHR(device.present_queue, &present_info);


		current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

		gettimeofday(&timer_end, nullptr);


		double dt = timer_end.tv_sec - timer_start.tv_sec + (timer_end.tv_usec - timer_start.tv_usec);
		printf("frame dt: %f\n", (dt / 1000000.0f));
	}


	// Test function adapted from sasha's example screenshot
	void receive_swapchain_image(uint32_t image_index)
	{
		char *data;
		uint32_t memcpy_offset = 0;

		for(uint32_t i = 0; i < CLIENTHEIGHT; i++)
		{
			// Read from server
			uint32_t servbuf[1920 * 3];
			int server_read = read(client.socket_fd, servbuf, 1920 * 3);
			//printf("Read from server\n");

			if(server_read != -1)
			{
				// Map the image buffer memory using char *data at the current memcpy offset based on the current read
				vkMapMemory(device.logical_device, image_buffer_memory, memcpy_offset, 1920 * 3, 0, (void **) &data);
				memcpy(data, servbuf, 1920 * 3);
				vkUnmapMemory(device.logical_device, image_buffer_memory);

				// Increase the memcpy offset to be representative of the next row's pixels
				memcpy_offset += 1920 * 3;

				write(client.socket_fd, "linedone", 8);
			}
		}

		// Now the VkBuffer should be filled with memory that we can copy to a swapchain image.
		// Transition swapchain image to copyable layout
		VkCommandBuffer copy_cmdbuf = begin_command_buffer(device, command_pool);

		// Transition current swapchain image to be transfer_dst_optimal. Need to note the src and dst access masks
		transition_image_layout(device, command_pool, copy_cmdbuf,
								swapchain.images[image_index],
								VK_ACCESS_MEMORY_READ_BIT,			  // src access_mask
								VK_ACCESS_TRANSFER_WRITE_BIT,		  // dst access_mask
								VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,	  // current layout
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // new layout to transfer to (destination)
								VK_PIPELINE_STAGE_TRANSFER_BIT,		  // dst pipeline mask
								VK_PIPELINE_STAGE_TRANSFER_BIT);	  // src pipeline mask

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
							   image_buffer,
							   swapchain.images[image_index],
							   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							   1, &copy_region);

		printf("Copy command buffer performed\n");

		// Transition swapchain image back
		transition_image_layout(device, command_pool, copy_cmdbuf,
								swapchain.images[image_index],
								VK_ACCESS_TRANSFER_WRITE_BIT,		  // src access mask
								VK_ACCESS_MEMORY_READ_BIT,			  // dst access mask
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // current layout
								VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,	  // layout transitioning to
								VK_PIPELINE_STAGE_TRANSFER_BIT,		  // pipeline flags
								VK_PIPELINE_STAGE_TRANSFER_BIT);	  // pipeline flags

		end_command_buffer(device, command_pool, copy_cmdbuf);
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