# offloaded-vulkan-tests
Very lightweight test engine for offloaded rendering using Vulkan. A lot of it is adapted either from vulkan-tutorial, vkguide, and sascha's vulkan samples.

At a high level overview, the server will render a high fidelity central region of the viewport and send that rendered frame over a TCP socket to the client.
The client renders the entire scene, but at a lower resolution and upscales with foveated rendering. 
The server frame is rendered at the center of the client's frame with a fullscreen quad.

## Current State
In this day of our [Lord](https://youtu.be/BlSinvbNqIA?t=34), there are some features missing: First of all, input from client to server to move position and camera rotation is disabled, because I have very little async to do this without destroying runtime. The fullscreen quad also stretches out the server image to display it onto a 1920x1080, which looks awful and is on the TODO. Foveated rendering is unimplemented. Everything described is the ideal to-come description, although much of it still applies in the current state.

Streaming the entire frame, fully rendered, is typically not possible due to consumer bandwidth limitations, so workarounds must be used.
These can be:
- Converting the rendered image to video frames on the server and sending them to a client (see: A Log-Rectilinear Transformation for Foveated 360-degree Video Streaming, and many others along this line of work)
- Neural rendering can be employed to upscale a lower resolution video (see: Facebook's DeepFovea). 

These typically have a high powered GPU working on the server to perform the inititial rendering of the subsampled frame, and then have a lot of even more high-powered GPU's to perform some kind of TSAA-style inference -- the problem with this is that they also require some very complex, trained AI, and need a lot of compute power to do the inference in real-time.

## Advantages of this approach
- Video frames are going to be lower fidelity, which can be extra unpleasant in VR and lose some details critical for gaming, so showing rendered frames for the highest quality is ideal.
- This is a generalizable approach that should be implementable into a lot of applications, and utilizes two GPU's (essentially) to do some load scheduling.

## Probable Disadvantages of this approach
- To compute the same frame for the server and the client, they both need to keep a copy of the game's vertex data. 
- In the context of a game with stochastic behaviour, this means that the stochastic behaviour must be modeled across both "copies" of the game, increasing the bandwidth beyond just the rendered frames - that or send any random variables across the network as well. 
- Mouse and keyboard I/O from the client to server will probably need some sort of blocking to upload camera position and UBO's.
This is annoying and slow if single-threaded, and not exactly trivial to multithread.

## Vulkan Walkthrough
Here, I'll detail the important parts of the vulkan setup.

The server goes through a normal rendering loop, with only one minimum renderpass and one minimum set of shaders. 
After the frame has been rendered (and presented through the swapchain, although that's not strictly necessary), the swapchain image is copied to a CPU-visible buffer and sent over the network to the client. To avoid sending extra packets (alpha values primarily), the CPU will go through a vectorized loop and remove the alpha values. This turned out to not be doable in glsl, since vec3 buffers are the same size as vec4's. There may be a way to do this in OpenCL with some Vulkan extensions, but those have not yet been explored.

The client will do a first renderpass that generates a low quality frame. A second renderpass runs after it receives the server's swapchain image, and it will display the two frames together, with the server's frame comprising the center pixels of the second renderpass, which outputs to the client's swapchain. 

In the subsequent subsections, I will describe the changes in Vulkan (with code) that need to be changed to get this working.

### **Swapchain** 
The server's swapchain should be created with the following imageUsage:
```cpp
VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
```

```VK_IMAGE_USAGE_TRANSFER_DST_BIT``` isn't really necessary unless you're copying a buffer or image directly to the swapchain image - which an older implementation did.
This is defined in ``vk_swapchain.h``

### **Offscreen Pass** (Client)
The client will need an extra renderpass.
The first renderpass is an offscreen pass, meaning it won't render to the swapchain (its output won't be presented to the screen), but to another sampler which will be used as input for the second renderpass -- so this offscreen pass goes through all the vertex data and renders the subsampled, full image.
This offscreen renderpass can be organized neatly like so:

```cpp
// Modeled from sascha's radialblur
struct
{
	VkFramebuffer framebuffer;
	VulkanAttachment colour_attachment;
	VulkanAttachment depth_attachment;
	VkRenderPass renderpass;
	VkSampler sampler;
} offscreen_pass;
```

Creating this renderpass is pretty straightforward; first, the colour and depth attachment descriptions and references are defined with the appropriate formats and whatnot. 
Two subpass dependencies are used, and then the renderpass can be created with the defined ``VulkanAttachment``'s. 
Finally, the offscreen pass's framebuffer can be created, again using the two VulkanAttachment's image_views. 
The ``sampler`` is used as input for the next renderpass, which will render to the swapchain.
The setup for the entire offscreen pass happens in the ``setup_offscreen()`` function in ``client.cpp``.

### **Fullscreen Quad Pass** (client)
The fullscreen quad pass is very simple -- it takes in two ``COMBINED_IMAGE_SAMPLER``'s and generates a fullscreen quad triangle which is then clipped to the viewport. 
Sascha Willems has a great explanation [here](https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/)
The two image sampler's come from the server's frame that was sent to the client, and from the first renderpass on the client that generated the low-quality image (which will be foveated in the future).

Currently, since the server image wasn't being clipped properly in the fragment shader, it's instead copied into the client's image and that is what's displayed. The fullscreen quad pass is nevertheless kept to make it be easy to correct this, as this incurs a runtime penalty.

```cpp
			int32_t start_col_pixel = (1920 / 2 - 512 / 2);
			int32_t start_row_pixel = (1080 / 2 - 512 / 2);
			VkOffset3D image_offset = {
				.x = start_col_pixel,
				.y = start_row_pixel,
				.z = 0,
			};

			// Create the vkbufferimagecopy pregions
			VkBufferImageCopy copy_region = {
				.bufferOffset	   = 0,
				.bufferRowLength   = 0,
				.bufferImageHeight = 0,
				.imageSubresource  = image_subresource,
				.imageOffset	   = image_offset,
				.imageExtent	   = {SERVERWIDTH, SERVERHEIGHT, 1},
			};


			// Perform the copy
			vkCmdCopyBufferToImage(command_buffers[i],
								   image_buffer,
								   offscreen_pass.colour_attachment.image,
								   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								   1, &copy_region);
```

### **Server Frame Sampler Setup** (client)
The image used with the sampler is created with usage ``VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT``, since that lets us copy to it (which is necessary when reading over the network), and also use it as input from which to sample. 
It's then transitioned to ``VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL``, and to ``VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`` when reading from the network.
This occurs in ``setup_serverframe_sampler()`` of ``client.cpp``.

### **Graphics Pipeline Differences between model pipeline and fullscreen quad** (client)
The code in ``setup_graphics_pipeline()`` of ``client.cpp`` is fairly straightforward -- two graphics pipelines are created, one for each renderpass, and are much more similar than not. A few differences arise though. 
The model pipeline uses the offscreen pass's renderpass, and has vertex input to the shader, while the fullscreen quad's pipeline has an empty vertex input state.
The most important difference is probably that the model pipeline uses ``VK_CULL_MODE_BACK_BIT``, and the fullscreen quad shader uses ``VK_CULL_MODE_FRONT_BIT`` for its pipeline rasterizer.

### **Networked Frames**
The frame from the server is sent over the network. This section will describe that painful and ugly process.

We define an ImagePacket struct:
```cpp
struct ImagePacket
{
	VkImage image;
	VkDeviceMemory memory;
	VkSubresourceLayout subresource_layout;
	char *data;

	void map_memory(VulkanDevice device)
	{
		vkMapMemory(device.logical_device, memory, 0, VK_WHOLE_SIZE, 0, (void **) &data);
		data += subresource_layout.offset;
		vkUnmapMemory(device.logical_device, memory);
	}

	void destroy(VulkanDevice device)
	{
		vkFreeMemory(device.logical_device, memory, nullptr);
		vkDestroyImage(device.logical_device, image, nullptr);
	}
};
```

In the ``copy_swapchain_image()`` function of ``main.cpp``, this is used. 
It creates an image (and this runs every frame, so creating the image beforehand is probably going to be one of those micro-optimizations that should be done at some point, and just reuse that image by overwriting the memory each frame instead), transitions it to an appropriate layout to be copyable, and copies the swapchain image to it. 
The transition setup is important, but the actual copy itself is done with:

```cpp
vkCmdCopyImage(copy_cmdbuffer,
			   src_image,
			   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			   dst.image,
			   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			   1,
			   &image_copy_region);
```

This is done in the main renderloop after it's presented.
```cpp
vkQueuePresentKHR(device.present_queue, &present_info);
ImagePacket image_packet = copy_swapchain_image();
```

This char *data in the ``ImagePacket`` struct is a little sus. It's a pointer to the underlying image memory, but it seems it only points to one row of the image at a time, and has to be incremented by the subresource layout's rowPitch.

The imagepacket data is originally packing ``R8G8B8A8`` values into a ``uint32_t``. The vectorized ``rgba_to_rgb`` (utils.h) unpacks those very quickly to reduce network latency. 

```cpp
void send_image_to_client(ImagePacket image_packet)
{
	size_t output_framesize_bytes = SERVERWIDTH * SERVERHEIGHT * 3;
	size_t input_framesize_bytes	   = SERVERWIDTH * SERVERHEIGHT * sizeof(uint32_t);

	uint8_t sendpacket[output_framesize_bytes];
	rgba_to_rgb((uint8_t *) image_packet.data, sendpacket, input_framesize_bytes);
	send(server.client_fd, sendpacket, output_framesize_bytes, 0);
}

```



Over on the client's side, ``receive_swapchain_image()`` will retrieve the swapchain image that was sent by the server. It ends up having to copy back in the alpha values though, in order to easily input it as a sampler.

```cpp
		// Create buffer to read from tcp socket
		VkDeviceSize num_bytes_network_read = SERVERWIDTH * SERVERHEIGHT * 3;
		VkDeviceSize num_bytes_for_image	= SERVERWIDTH * SERVERHEIGHT * sizeof(uint32_t);
		uint8_t servbuf[num_bytes_network_read];

		// Begin mapping the memory
		vkMapMemory(dr->device.logical_device, dr->image_buffer_memory, 0, num_bytes_for_image, 0, (void **) &dr->server_image_data);

		int server_read = recv(dr->client.socket_fd, servbuf, num_bytes_network_read, MSG_WAITALL);

		// Convert RGB network packets to RGBA
		if(server_read != -1)
		{
			rgb_to_rgba(servbuf, dr->server_image_data, num_bytes_for_image);
		}

		vkUnmapMemory(dr->device.logical_device, dr->image_buffer_memory);
```
The RGB values from the server are read into a ``uint8_t`` buffer containing one slot for each byte of data (with RGB comprising 3 bytes). This is then converted to 4-byte values with an alpha component added back in, encoded into a ``char *data`` pointer. The VkBuffer image memory is mapped during this to that pointer. This is just copying it into a *buffer* though, which needs to be copied into an image that can be read in for our next renderpass via a fullscreen quad.

This is done in a very similar way to how the swapchain image was copied on the server side.
The colour attachment's image is transitioned to an appropriate layout (``VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL``), copied to from a VkBuffer that had its memory mapped to that ``uint8_t`` buffer, and then the image is transitioned back to be read by the shader.


### **Putting Everything Together (command buffer setup)** (client)
``setup_command_buffers()`` in ``client.cpp`` is where the two renderpasses happen.
Basically, the first renderpass happens with the model's pipeline, with vertex and index buffers bound, and it draws to the framebuffer for the offscreen pass.
We use ``vkCmdDrawIndexed`` to take advantage of the ibo.

In the second renderpass, it renders a fullscreen quad directly to the swapchain, using the appropriately setup fullscreen quad pipeline. It only draws three triangles, on which the two samplers from the server and previous renderpass are displayed.

The code for this rendering loop is very simple, because in Vulkan, the bulk of the hard stuff happens elsewhere, outside the main rendering loop.

Two threads are used on the client: In one, it performs the first renderpass to render the scene as a whole while the other fetches the server's frame over the network. Once they join, the fullscreen quad pass runs.	

Here it is running. Locally, it gets 60fps (vsync) just fine. On a consumer-grade network, it can get around 41 fps on around a 250Mbit/s wifi connection.

![Running frame](https://github.com/lilinitsy/offloaded-vulkan-tests/blob/separate-renderpasses/screenshots/alphascrn.png)

#### **Models**
The models come from a 3D scan of a friend of mine (rendered by the server), and from a 3D scan of my lab desk. Yay! Each one is something like 30-40k vertices. And my shitty engine only supports one model.

## Issues To Be Fixed
Notable issues that should be fixed include:
- ~~Sending the server frame as one 512x512 packet instead of scanline packets~~ Done.
- Test sending differently sized tiles, including perhaps not updating every part of the image, but only portions that change.
- Async on the server to have one thread perform the copy and send, one thread handling rendering, and one thread waiting on UBO input from the client (mouse, keyboard) to reduce the overhead from a serial pipeline
- ~~Async on the client to have one thread read the sampler from the server, one thread performing the rendering (and waiting on the first thread after the first renderpass)~~, and one thread possibly to send the UBO's over. (Partially done, the UBO's being sent are currently unhandled).
- Fix the RGB-BGR translation that happens when the server's frame is sent to the client (minor, unconcerned)
- ~~Render the client's frame at a smaller resolution and have it upscaled in the second renderpass to perform some sort of foveated rendering.~~ Partially done, will come back to it
- ~~On the server, don't bother rendering to the swapchain images, and render to an RGB image to cut out the overhead from the probably unimportant alpha component.~~
