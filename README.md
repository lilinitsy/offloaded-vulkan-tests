# offloaded-vulkan-tests
Very lightweight test engine for offloaded rendering using Vulkan. A lot of it is adapted either from vulkan-tutorial, vkguide, and sascha's vulkan samples.

At a high level overview, the server will render a high fidelity central region of the viewport and send that rendered frame over a TCP socket to the client.
The client renders the entire scene, but at a lower resolution and upscales with foveated rendering. 
The server frame is rendered at the center of the client's frame with a fullscreen quad.

## Current State
In this day of our [Lord](https://youtu.be/BlSinvbNqIA?t=34), there are some features missing: First of all, input from client to server to move position and camera rotation is disabled, because I have very little async to do this without destroying runtime. The fullscreen quad also stretches out the server image to display it onto a 1920x1080, which looks awful and is on the TODO. Foveated rendering is unimplemented. Running over a local TCP socket is fine, but over an actual network seems to produce odd image artifacting bugs. Everything described is the ideal to-come description, although much of it still applies in the current state.

Streaming the entire frame, fully rendered, is typically not possible due to consumer bandwidth limitations, so workarounds must be used.
These can be:
- Converting the rendered image to video frames on the server and sending them to a client (see: A Log-Rectilinear Transformation for Foveated 360-degree Video Streaming, and many others along this line of work)
- Neural rendering can be employed to upscale a lower resolution video (see: Facebook's DeepFovea). 
These typically have a high powered GPU working on the server to perform the inititial rendering of the subsampled frame, and then have a lot of even more high-powered GPU's to perform some kind of TSAA-style inference -- the problem with this is that they also require some very complex, trained AI, and need a lot of compute power to do the inference in real-time.

## Advantages of this approach
- Video frames are going to be lower fidelity, which can be extra unpleasant in VR, so showing rendered frames for the highest quality is ideal.
- This is a generalizable approach that should be implementable into a lot of applications, and utilizes two GPU's (essentially) to do some load scheduling.

## Probable Disadvantages of this approach
- To compute the same frame for the server and the client, they both need to keep a copy of the game's vertex data. 
In the context of a game with stochastic behaviour, this means that the stochastic behaviour must be modeled across both "copies" of the game, increasing the bandwidth beyond just the rendered frames. 
- Mouse and keyboard I/O from the client to server will probably need some sort of blocking to upload camera position and UBO's.
This is annoying and slow if single-threaded (like it is rn), and fuck async in C++.

## Vulkan Walkthrouh
Here, I'll detail the important parts of the vulkan setup.

The server goes through a normal rendering loop, with only one minimum renderpass and one minimum set of shaders. 
After the frame has been rendered (and presented through the swapchain, although that's not strictly necessary), the swapchain image is copied to a CPU-visible buffer and sent over the network to the client. 

The client will do a first renderpass that generates a low quality frame. A second renderpass should run after it receives the server's swapchain image, and it will display the two frames together, with the server's frame comprising the center pixels of the second renderpass, which outputs to the client's swapchain. 

In the subsequent subsections, I will describe the changes in Vulkan (with code) that need to be changed to get this working.

### **Swapchain** 
The server's swapchain should be created with the following imageUsage:
```cpp
VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
```

```VK_IMAGE_USAGE_TRANSFER_DST_BIT``` isn't really necessary unless you're copying a buffer or image directly to the swapchain image - which an older implementation did.
This is defined in ``vk_swapchain.h``

### **Offscreen Pass** (Client)
The client will need an extra renderpass, or a subpass using input attachments, which I opted not to use. 
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
Two subpass dependencies are used, and then the renderpass can be created with the defined VulkanAttachment's. 
Finally, the offscreen pass's framebuffer can be created, again using the two VulkanAttachment's image_views. 
The ``sampler`` is used as input for the next renderpass, which will render to the swapchain.
The setup for the entire offscreen pass happens in the ``setup_offscreen()`` function in ``client.cpp``.

### **Fullscreen Quad Pass** (client)
The fullscreen quad pass is very simple -- it takes in two ``COMBINED_IMAGE_SAMPLER``'s and generates a fullscreen quad triangle which is then clipped to the viewport. 
Sascha Willems has a great explanation [here](https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/)
The two image sampler's come from the server's frame that was sent to the client, and from the first renderpass on the client that generated the low-quality image (which will be foveated in the future).

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
	}

	void destroy(VulkanDevice device)
	{
		vkUnmapMemory(device.logical_device, memory);
		vkFreeMemory(device.logical_device, memory, nullptr);
		vkDestroyImage(device.logical_device, image, nullptr);
	}
};
```

In the ``copy_swapchain_image()`` function of ``main.cpp``, this is used. 
So it creates an image (and this runs every frame, so creating the image beforehand is probably going to be one of those micro-optimizations that should be done at some point, and just reuse that image by overwriting the memory each frame instead), transitions it to an appropriate layout to be copyable, and copies the swapchain image to it. 
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

This char *data in the ``ImagePacket`` struct is a little sus. It's a pointer to the underlying image memory, but it seems it only points to one row of the image at a time, and has to be incremented by the subresource layout's rowPitch. The code to send the packet is then broken up into a for loop, sending one line of the image at a time, which creates more overhead.

```cpp
for(uint32_t i = 0; i < SERVERHEIGHT; i++)
{
	// Send scanline
	uint32_t *row = (uint32_t *) image_packet.data;
	send(server.client_fd, row, SERVERWIDTH * sizeof(uint32_t), 0);

	// Receive code that line has been written
	char code[8];
	int client_read = read(server.client_fd, code, 8);
	image_packet.data += image_packet.subresource_layout.rowPitch;
}
```

The server waits for the client to send a code that it has read the line before proceeding to the next. The branch ``scaline-optimization`` has work to correct this, although is currently incomplete.

Over on the client's side, ``receive_swapchain_image()`` will retrieve the swapchain image that was sent by the server.

```cpp
uint32_t servbuf[SERVERWIDTH];
VkDeviceSize num_bytes = SERVERWIDTH * sizeof(uint32_t);

// Fetch server frame
for(uint32_t i = 0; i < SERVERHEIGHT; i++)
{
	// Read from server
	int server_read = read(client.socket_fd, servbuf, SERVERWIDTH * sizeof(uint32_t));

	if(server_read != -1)
	{
		// Map the image buffer memory using char *data at the current memcpy offset based on the current read
		vkMapMemory(device.logical_device, image_buffer_memory, memcpy_offset, num_bytes, 0, (void **) &data);
		memcpy(data, servbuf, (size_t) num_bytes);
		vkUnmapMemory(device.logical_device, image_buffer_memory);

		// Increase the memcpy offset to be representative of the next row's pixels
		memcpy_offset += num_bytes;

		// Send next row num back for server to print out
		uint32_t pixelnum	= i + memcpy_offset / num_bytes;
		std::string strcode = std::to_string(pixelnum);
		char *code			= (char *) strcode.c_str();
		write(client.socket_fd, code, 8);
	}
}
```
This is read into a uint32_t buffer of size SERVERWIDTH, which is equivalent to one row. 
Each row encoded by the server is read from a ``char *data`` pointer; but we're reading it into a uint32_t pointer. How??? Well, the bytes match. 
``uint32_t`` is 4 bytes, or representative of one pixel. 
The memory is then mapped for that line to a ``VkBuffer``, and so on for each line of the swapchain image being sent to the client. 
This is just copying it into a *buffer* though, which needs to be copied into an image that can be read in for our next renderpass via a fullscreen quad.

This is done in a very similar way to how the swapchain image was copied on the server side.
The colour attachment's image is transitioned to an appropriate layout (``VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL``), copied to from a VkBuffer that had its memory mapped to that ``uint32_t`` buffer, and then the image is transitioned back to be read by the shader.


### **Putting Everything Together (command buffer setup)** (client)
``setup_command_buffers()`` in ``client.cpp`` is where the two renderpasses happen.
Basically, the first renderpass happens with the model's pipeline, with vertex and index buffers bound, and it draws to the framebuffer for the offscreen pass.
We use ``vkCmdDrawIndexed`` to take advantage of the ibo.

In the second renderpass, it renders a fullscreen quad directly to the swapchain, using the appropriately setup fullscreen quad pipeline. It only draws three triangles, on which the two samplers from the server and previous renderpass are displayed.

The code for this rendering loop is very simple, because in Vulkan, the bulk of the hard stuff happens elsewhere, outside the main rendering loop.

Here it is running. Locally, it gets 60fps (vsync) just fine. On a consumer-grade network, it got around 26fps, with some weird memory / synchronization issues over the network making the server display quite buggy.

![Running frame](https://github.com/lilinitsy/offloaded-vulkan-tests/blob/separate-renderpasses/screenshots/alphascrn.png)

#### **Models**
The models come from a 3D scan of a friend of mine (rendered by the server), and from a 3D scan of my lab desk. Yay! Each one is something like 30-40k vertices. And my shitty engine only supports one model.

## Issues To Be Fixed
Notable issues that should be fixed include:
- Sending the server frame as one 512x512 packet instead of scanline packets
- Async on the server to have one thread perform the copy and send, one thread handling rendering, and one thread waiting on UBO input from the client (mouse, keyboard) to reduce the overhead from a serial pipeline
- Async on the client to have one thread read the sampler from the server, one thread performing the rendering (and waiting on the first thread after the first renderpass), and one thread possibly to send the UBO's over.
- Fix the RGB-BGR translation that happens when the server's frame is sent to the client (minor, unconcerned)
- Render the client's frame at a smaller resolution and have it upscaled in the second renderpass to perform some sort of foveated rendering
- "rewrite it in rust"????