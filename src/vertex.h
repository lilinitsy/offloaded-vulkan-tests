#ifndef VERTEX_H
#define VERTEX_H


#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>

#include "vk_buffers.h"
#include "vk_device.h"
#include "vk_initializers.h"




struct Vertex
{
	glm::vec3 position;
	glm::vec3 colour;
	glm::vec2 texcoord;

	// Vulkan format for the vertex shader
	static VkVertexInputBindingDescription get_binding_description()
	{
		VkVertexInputBindingDescription binding_desc = vki::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
		return binding_desc;
	}

	static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributes;
		attributes.resize(3);

		// vertex position
		attributes[0] = vki::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position));
		// vertex colour
		attributes[1] = vki::vertexInputAttributeDescription(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, colour));
		// texture coordinates
		attributes[2] = vki::vertexInputAttributeDescription(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texcoord));

		return attributes;
	}

	bool operator==(const Vertex &v) const
	{
		return position == v.position && colour == v.colour && texcoord == v.texcoord;
	}
};

// Bullshit for map
namespace std
{
	template<>
	struct hash<Vertex>
	{
		size_t operator()(Vertex const &vertex) const
		{
			return ((hash<glm::vec3>()(vertex.position) ^
					 (hash<glm::vec3>()(vertex.colour) << 1)) >>
					1) ^
				   (hash<glm::vec2>()(vertex.texcoord) << 1);
		}
	};
} // namespace std


void initialize_vertex_buffers(VulkanDevice device, std::vector<Vertex> vertices, VkBuffer *vbo, VkDeviceMemory *vbo_mem, VkCommandPool command_pool)
{
	// Staging buffer to use the host visible as temp buffer, and then a device local one on the gpu
	VkDeviceSize buffersize = sizeof(vertices[0]) * vertices.size();
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	create_buffer(device, buffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	// Map buffer memory
	void *data;
	vkMapMemory(device.logical_device, staging_buffer_memory, 0, buffersize, 0, &data);
	memcpy(data, vertices.data(), buffersize);
	vkUnmapMemory(device.logical_device, staging_buffer_memory);

	// copy device
	create_buffer(device, buffersize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *vbo, *vbo_mem);
	copy_buffer(device, staging_buffer, *vbo, command_pool, buffersize);
	vkDestroyBuffer(device.logical_device, staging_buffer, nullptr);
	vkFreeMemory(device.logical_device, staging_buffer_memory, nullptr);
}

void initialize_index_buffers(VulkanDevice device, std::vector<uint32_t> indices, VkBuffer *ibo, VkDeviceMemory *ibo_mem, VkCommandPool command_pool)
{
	VkDeviceSize buffersize = sizeof(indices[0]) * indices.size();
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	create_buffer(device, buffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	// Map buffer memory
	void *data;
	vkMapMemory(device.logical_device, staging_buffer_memory, 0, buffersize, 0, &data);
	memcpy(data, indices.data(), buffersize);
	vkUnmapMemory(device.logical_device, staging_buffer_memory);

	// copy device
	create_buffer(device, buffersize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *ibo, *ibo_mem);
	copy_buffer(device, staging_buffer, *ibo, command_pool, buffersize);
	vkDestroyBuffer(device.logical_device, staging_buffer, nullptr);
	vkFreeMemory(device.logical_device, staging_buffer_memory, nullptr);
}



#endif