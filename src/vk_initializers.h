#ifndef VK_INITIALIZERS_H
#define VK_INITIALIZERS_H


#include <vulkan/vulkan.h>

// Use camel case for this function names to match idiomatic vulkan


namespace vki
{
	inline VkInstanceCreateInfo instanceCreateInfo()
	{
		VkInstanceCreateInfo instance_create_info = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		};

		return instance_create_info;
	}

	VkPresentInfoKHR presentInfoKHR(uint32_t waitSemaphoreCount, const VkSemaphore *pWaitSemaphores, uint32_t swapchainCount, const VkSwapchainKHR *pSwapchains, const uint32_t *pImageIndices)
	{
		VkPresentInfoKHR present_info_KHR = {
			.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores	= pWaitSemaphores,
			.swapchainCount		= swapchainCount,
			.pSwapchains		= pSwapchains,
			.pImageIndices		= pImageIndices,
		};

		return present_info_KHR;
	}


	//////////////////// VIEWPORT

	inline VkViewport viewport(float x, float y, float width, float height, float minDepth, float maxDepth)
	{
		VkViewport viewport = {
			.x		  = x,
			.y		  = y,
			.width	  = width,
			.height	  = height,
			.minDepth = minDepth,
			.maxDepth = maxDepth,
		};

		return viewport;
	}

	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(uint32_t viewportCount, const VkViewport *pViewports, uint32_t scissorCount, const VkRect2D *pScissors)
	{
		VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {
			.sType		   = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = viewportCount,
			.pViewports	   = pViewports,
			.scissorCount  = scissorCount,
			.pScissors	   = pScissors,
		};

		return pipeline_viewport_state_create_info;
	}


	inline VkRect2D rect2D(VkOffset2D offset, VkExtent2D extent)
	{
		VkRect2D rect_2D = {
			.offset = offset,
			.extent = extent,
		};

		return rect_2D;
	}


	//////////////////// VERTEX INPUT
	inline VkVertexInputBindingDescription vertexInputBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
	{
		VkVertexInputBindingDescription vertex_input_binding_description = {
			.binding   = binding,
			.stride	   = stride,
			.inputRate = inputRate,
		};

		return vertex_input_binding_description;
	}

	inline VkVertexInputAttributeDescription vertexInputAttributeDescription(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset)
	{
		VkVertexInputAttributeDescription vertex_input_attribute_description = {
			.location = location,
			.binding  = binding,
			.format	  = format,
			.offset	  = offset,
		};

		return vertex_input_attribute_description;
	}

	inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(uint32_t vertexBindingDescriptionCount, const VkVertexInputBindingDescription *pVertexBindingDescriptions, uint32_t vertexAttributeDescriptionCount, const VkVertexInputAttributeDescription *pVertexAttributeDescriptions)
	{
		VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = {
			.sType							 = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount	 = vertexBindingDescriptionCount,
			.pVertexBindingDescriptions		 = pVertexBindingDescriptions,
			.vertexAttributeDescriptionCount = vertexAttributeDescriptionCount,
			.pVertexAttributeDescriptions	 = pVertexAttributeDescriptions,
		};

		return pipeline_vertex_input_state_create_info;
	}


	//////////////////// COMMAND BUFFERS
	inline VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex)
	{
		VkCommandPoolCreateInfo command_pool_create_info = {
			.sType			  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = queueFamilyIndex,
		};

		return command_pool_create_info;
	}

	inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(const void *pNext, VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t commandBufferCount)
	{
		VkCommandBufferAllocateInfo command_buffer_allocate_info = {
			.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool		= commandPool,
			.level				= level,
			.commandBufferCount = commandBufferCount,
		};

		return command_buffer_allocate_info;
	}

	// probably never gonna use the pInheritence info, don't touch it
	inline VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags)
	{
		VkCommandBufferBeginInfo command_buffer_begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = flags,
		};

		return command_buffer_begin_info;
	}

	inline VkCommandBufferBeginInfo commandBufferBeginInfo()
	{
		VkCommandBufferBeginInfo command_buffer_begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		};

		return command_buffer_begin_info;
	}

	inline VkSubmitInfo submitInfo()
	{
		VkSubmitInfo submit_info = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		};

		return submit_info;
	}


	//////////////////// DEVICE
	// not going to give any flags, ignore for now, overload if I do
	inline VkDeviceQueueCreateInfo deviceQueueCreateInfo(uint32_t queueFamilyIndex, uint32_t queueCount, const float *pQueuePriorities)
	{
		VkDeviceQueueCreateInfo device_queue_create_info = {
			.sType			  = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queueFamilyIndex,
			.queueCount		  = queueCount,
			.pQueuePriorities = pQueuePriorities,
		};

		return device_queue_create_info;
	}

	// not going to use flags, just ignore it, overload if I do
	inline VkDeviceCreateInfo deviceCreateInfo(uint32_t queueCreateInfoCount, const VkDeviceQueueCreateInfo *pQueueCreateInfos, uint32_t enabledLayerCount, const char *const *ppEnabledLayerNames, uint32_t enabledExtensionCount, const char *const *ppEnabledExtensionNames, const VkPhysicalDeviceFeatures *pEnabledFeatures)
	{
		VkDeviceCreateInfo device_create_info = {
			.sType					 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount	 = queueCreateInfoCount,
			.pQueueCreateInfos		 = pQueueCreateInfos,
			.enabledLayerCount		 = enabledLayerCount,
			.ppEnabledLayerNames	 = ppEnabledLayerNames,
			.enabledExtensionCount	 = enabledExtensionCount,
			.ppEnabledExtensionNames = ppEnabledExtensionNames,
			.pEnabledFeatures		 = pEnabledFeatures,
		};

		return device_create_info;
	}


	//////////////////// BUFFERS
	// not going to use flags
	// not going to use queueFamilyIndexCount
	// not going to use pQueueFamilyIndices -- overload if necessary
	inline VkBufferCreateInfo bufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode)
	{
		VkBufferCreateInfo buffer_create_info = {
			.sType		 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size		 = size,
			.usage		 = usage,
			.sharingMode = sharingMode,
		};

		return buffer_create_info;
	}


	inline VkMemoryAllocateInfo memoryAllocateInfo(VkDeviceSize allocationSize, uint32_t memoryTypeIndex)
	{
		VkMemoryAllocateInfo memory_allocate_info = {
			.sType			 = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize	 = allocationSize,
			.memoryTypeIndex = memoryTypeIndex,
		};

		return memory_allocate_info;
	}


	//////////////////// SHADERS / PIPELINES / RENDERPASS

	// Only declare stype, everything else can probably be explicitly stated in its creation
	inline VkRenderPassCreateInfo renderPassCreateInfo()
	{
		VkRenderPassCreateInfo render_pass_create_info = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		};

		return render_pass_create_info;
	}

	inline VkRenderPassBeginInfo renderPassBeginInfo(VkRenderPass renderPass, VkFramebuffer framebuffer, VkRect2D renderArea, uint32_t clearValueCount, const VkClearValue *pClearValues)
	{
		VkRenderPassBeginInfo renderpass_begin_info = {
			.sType			 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass		 = renderPass,
			.framebuffer	 = framebuffer,
			.renderArea		 = renderArea,
			.clearValueCount = clearValueCount,
			.pClearValues	 = pClearValues,
		};

		return renderpass_begin_info;
	}

	inline VkRenderPassBeginInfo renderPassBeginInfo(VkRenderPass renderPass, VkFramebuffer framebuffer, VkOffset2D offset, VkExtent2D extent, uint32_t clearValueCount, const VkClearValue *pClearValues)
	{
		VkRect2D renderArea = rect2D(offset, extent);

		VkRenderPassBeginInfo renderpass_begin_info = {
			.sType			 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass		 = renderPass,
			.framebuffer	 = framebuffer,
			.renderArea		 = renderArea,
			.clearValueCount = clearValueCount,
			.pClearValues	 = pClearValues,
		};

		return renderpass_begin_info;
	}


	inline VkShaderModuleCreateInfo shaderModuleCreateInfo(size_t codeSize, const uint32_t *pCode)
	{
		VkShaderModuleCreateInfo shader_module_create_info = {
			.sType	  = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = codeSize,
			.pCode	  = pCode,
		};

		return shader_module_create_info;
	}


	// Ignore pSpecializationInfo and flags, overload if necessary
	inline VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module, const char *pName = "main")
	{
		VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info = {
			.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage	= stage,
			.module = module,
			.pName	= pName, // entrance point
		};

		return pipeline_shader_stage_create_info;
	}


	inline VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable)
	{
		VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = {
			.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology				= topology,
			.primitiveRestartEnable = primitiveRestartEnable};

		return pipeline_input_assembly_state_create_info;
	}

	inline VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace)
	{
		VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = {
			.sType					 = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable		 = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode			 = polygonMode,
			.cullMode				 = cullMode,
			.frontFace				 = frontFace,
			.lineWidth				 = 1.0f,
		};

		return pipeline_rasterization_state_create_info;
	}

	inline VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(VkBool32 sampleShadingEnable, VkSampleCountFlagBits rasterizationSamples)
	{
		VkPipelineMultisampleStateCreateInfo pipeline_msaa_state_create_info = {
			.sType				  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable  = VK_FALSE,
		};

		return pipeline_msaa_state_create_info;
	}

	inline VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(VkBool32 blendEnable, VkColorComponentFlags colorWriteMask)
	{
		VkPipelineColorBlendAttachmentState pipeline_colour_blend_attachment_state = {
			.blendEnable	= blendEnable,
			.colorWriteMask = colorWriteMask,
		};

		return pipeline_colour_blend_attachment_state;
	}

	inline VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState *pAttachments)
	{
		VkPipelineColorBlendStateCreateInfo pipeline_colour_blend_state_create_info = {
			.sType			 = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable	 = VK_FALSE,
			.logicOp		 = VK_LOGIC_OP_COPY,
			.attachmentCount = attachmentCount,
			.pAttachments	 = pAttachments,
			.blendConstants	 = {0.0f, 0.0f, 0.0f, 0.0f},
		};

		return pipeline_colour_blend_state_create_info;
	}

	inline VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp, VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable, VkStencilOpState front, VkStencilOpState back, float minDepthBounds, float maxDepthBounds)
	{
		VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = {
			.sType				   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable	   = depthTestEnable,
			.depthWriteEnable	   = depthWriteEnable,
			.depthCompareOp		   = depthCompareOp,
			.depthBoundsTestEnable = depthBoundsTestEnable,
			.stencilTestEnable	   = stencilTestEnable,
			.front				   = front,
			.back				   = back,
			.minDepthBounds		   = minDepthBounds,
			.maxDepthBounds		   = maxDepthBounds,
		};

		return pipeline_depth_stencil_state_create_info;
	}

	inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(uint32_t setLayoutCount, const VkDescriptorSetLayout *pSetLayouts, uint32_t pushConstantRangeCount, const VkPushConstantRange *pPushConstantRanges)
	{
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
			.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount			= setLayoutCount,
			.pSetLayouts			= pSetLayouts,
			.pushConstantRangeCount = pushConstantRangeCount,
			.pPushConstantRanges	= pPushConstantRanges,
		};

		return pipeline_layout_create_info;
	}

	inline VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo()
	{
		VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		};

		return graphics_pipeline_create_info;
	}

	//////////////////// SWAPCHAIN / IMAGES

	// Only declare stype, everything else can probably be explicitly stated in its creation
	inline VkSwapchainCreateInfoKHR swapchainCreateInfoKHR()
	{
		VkSwapchainCreateInfoKHR swapchain_create_info_khr = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		};

		return swapchain_create_info_khr;
	}



	inline VkImageCreateInfo imageCreateInfo(VkImageCreateFlags flags, VkImageType imageType, VkFormat format, VkExtent3D extent, uint32_t mipLevels, uint32_t arrayLayers, VkSampleCountFlagBits samples, VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharingMode, VkImageLayout initialLayout)
	{
		VkImageCreateInfo image_create_info = {
			.sType		   = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags		   = flags,
			.imageType	   = imageType,
			.format		   = format,
			.extent		   = extent,
			.mipLevels	   = mipLevels,
			.arrayLayers   = arrayLayers,
			.samples	   = samples,
			.tiling		   = tiling,
			.usage		   = usage,
			.sharingMode   = sharingMode,
			.initialLayout = initialLayout,
		};

		return image_create_info;
	}

	inline VkImageViewCreateInfo imageViewCreateInfo()
	{
		VkImageViewCreateInfo image_view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		};

		return image_view_create_info;
	}

	inline VkImageMemoryBarrier imageMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkImage image, VkImageSubresourceRange subresourceRange)
	{
		VkImageMemoryBarrier image_memory_barrier = {
			.sType				 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask		 = srcAccessMask,
			.dstAccessMask		 = dstAccessMask,
			.oldLayout			 = oldLayout,
			.newLayout			 = newLayout,
			.srcQueueFamilyIndex = srcQueueFamilyIndex,
			.dstQueueFamilyIndex = dstQueueFamilyIndex,
			.image				 = image,
			.subresourceRange	 = subresourceRange,
		};

		return image_memory_barrier;
	}

	inline VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount)
	{
		VkImageSubresourceRange image_subresource_range = {
			.aspectMask		= aspectMask,
			.baseMipLevel	= baseMipLevel,
			.levelCount		= levelCount,
			.baseArrayLayer = baseArrayLayer,
			.layerCount		= layerCount,
		};

		return image_subresource_range;
	}

	inline VkImageSubresourceLayers imageSubresourceLayers(VkImageAspectFlags aspectMask, uint32_t mipLevel, uint32_t baseArrayLevel, uint32_t layerCount)
	{
		VkImageSubresourceLayers image_subresource_layers = {
			.aspectMask		= aspectMask,
			.mipLevel		= mipLevel,
			.baseArrayLayer = baseArrayLevel,
			.layerCount		= layerCount,
		};

		return image_subresource_layers;
	}

	inline VkSamplerCreateInfo samplerCreateInfo(VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW, float mipLodBias, VkBool32 anisotropyEnable, float maxAnisotropy, VkBool32 compareEnable, VkCompareOp compareOp, float minLod, float maxLod, VkBorderColor borderColor, VkBool32 unnormalizedCoordinates)
	{
		VkSamplerCreateInfo sampler_create_info = {
			.sType					 = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter				 = magFilter,
			.minFilter				 = minFilter,
			.mipmapMode				 = mipmapMode,
			.addressModeU			 = addressModeU,
			.addressModeV			 = addressModeV,
			.addressModeW			 = addressModeW,
			.mipLodBias				 = mipLodBias,
			.anisotropyEnable		 = anisotropyEnable,
			.maxAnisotropy			 = maxAnisotropy,
			.compareEnable			 = compareEnable,
			.compareOp				 = compareOp,
			.minLod					 = minLod,
			.maxLod					 = maxLod,
			.borderColor			 = borderColor,
			.unnormalizedCoordinates = unnormalizedCoordinates,
		};

		return sampler_create_info;
	}

	inline VkSamplerCreateInfo samplerCreateInfo(VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW, float mipLodBias, float maxAnisotropy, float minLod, float maxLod, VkBorderColor borderColor)
	{
		VkSamplerCreateInfo sampler_create_info = {
			.sType		   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter	   = magFilter,
			.minFilter	   = minFilter,
			.mipmapMode	   = mipmapMode,
			.addressModeU  = addressModeU,
			.addressModeV  = addressModeV,
			.addressModeW  = addressModeW,
			.mipLodBias	   = mipLodBias,
			.maxAnisotropy = maxAnisotropy,
			.minLod		   = 0.0f,
			.maxLod		   = 1.0f,
			.borderColor   = borderColor,
		};

		return sampler_create_info;
	}

	inline VkBufferImageCopy bufferImageCopy(VkDeviceSize bufferOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight, VkImageSubresourceLayers imageSubresource, VkOffset3D imageOffset, VkExtent3D imageExtent)
	{
		VkBufferImageCopy buffer_image_copy = {
			.bufferOffset	   = bufferOffset,
			.bufferRowLength   = bufferRowLength,
			.bufferImageHeight = bufferImageHeight,
			.imageSubresource  = imageSubresource,
		};

		return buffer_image_copy;
	}

	inline VkFramebufferCreateInfo framebufferCreateInfo(VkRenderPass renderPass, uint32_t attachmentCount, const VkImageView *pAttachments, uint32_t width, uint32_t height, uint32_t layers)
	{
		VkFramebufferCreateInfo framebuffer_create_info = {
			.sType			 = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass		 = renderPass,
			.attachmentCount = attachmentCount,
			.pAttachments	 = pAttachments,
			.width			 = width,
			.height			 = height,
			.layers			 = layers,
		};

		return framebuffer_create_info;
	}

	//////////////////// DESCRIPTORS
	inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags stageFlags, const VkSampler *pImmutableSamplers)
	{
		VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
			.binding			= binding,
			.descriptorType		= descriptorType,
			.descriptorCount	= descriptorCount,
			.stageFlags			= stageFlags,
			.pImmutableSamplers = pImmutableSamplers,
		};

		return descriptor_set_layout_binding;
	}

	inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(uint32_t bindingCount, const VkDescriptorSetLayoutBinding *pBindings)
	{
		VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
			.sType		  = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = bindingCount,
			.pBindings	  = pBindings,
		};

		return descriptor_set_layout_create_info;
	}

	inline VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSetLayout *pSetLayouts)
	{
		VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
			.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool		= descriptorPool,
			.descriptorSetCount = descriptorSetCount,
			.pSetLayouts		= pSetLayouts,
		};

		return descriptor_set_allocate_info;
	}


	inline VkDescriptorPoolSize descriptorPoolSize(VkDescriptorType type, uint32_t descriptorCount)
	{
		VkDescriptorPoolSize descriptor_pool_size = {
			.type			 = type,
			.descriptorCount = descriptorCount,
		};

		return descriptor_pool_size;
	}

	inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(uint32_t maxSets, uint32_t poolSizeCount, const VkDescriptorPoolSize *pPoolSizes)
	{
		VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
			.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets	   = maxSets,
			.poolSizeCount = poolSizeCount,
			.pPoolSizes	   = pPoolSizes,
		};

		return descriptor_pool_create_info;
	}

	inline VkDescriptorBufferInfo descriptorBufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		VkDescriptorBufferInfo descriptor_buffer_info = {
			.buffer = buffer,
			.offset = offset,
			.range	= range,
		};

		return descriptor_buffer_info;
	}

	inline VkDescriptorImageInfo descriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
	{
		VkDescriptorImageInfo descriptor_image_info = {
			.sampler	 = sampler,
			.imageView	 = imageView,
			.imageLayout = imageLayout,
		};

		return descriptor_image_info;
	}


	inline VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElement, uint32_t descriptorCount, const VkDescriptorType descriptorType, const VkDescriptorBufferInfo *pBufferInfo)
	{
		VkWriteDescriptorSet write_descriptor_set = {
			.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet			 = dstSet,
			.dstBinding		 = dstBinding,
			.dstArrayElement = dstArrayElement,
			.descriptorCount = descriptorCount,
			.descriptorType	 = descriptorType,
			.pBufferInfo	 = pBufferInfo, // only needed for descriptors that refer to buffer data
		};

		return write_descriptor_set;
	}

	inline VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElement, uint32_t descriptorCount, const VkDescriptorType descriptorType, const VkDescriptorImageInfo *pImageInfo)
	{
		VkWriteDescriptorSet write_descriptor_set = {
			.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet			 = dstSet,
			.dstBinding		 = dstBinding,
			.dstArrayElement = dstArrayElement,
			.descriptorCount = descriptorCount,
			.descriptorType	 = descriptorType,
			.pImageInfo		 = pImageInfo, // only needed for descriptors that refer to buffer data
		};

		return write_descriptor_set;
	}

	//////////////////// ASYNC OBJECTS

	inline VkSemaphoreCreateInfo semaphoreCreateInfo()
	{
		VkSemaphoreCreateInfo semaphore_create_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		return semaphore_create_info;
	}

	inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags)
	{
		VkFenceCreateInfo fence_create_info = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		return fence_create_info;
	}


} // namespace vki


#endif