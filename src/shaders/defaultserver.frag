#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_storage_buffer_object : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

layout(location = 0) in vec3 frag_colour;
layout(location = 1) in vec2 frag_texcoord;

layout(location = 0) out vec4 out_colour;

layout(binding = 1) uniform sampler2D tex_sampler;

layout(std140, binding = 2) restrict buffer WritePixelData
{
	u8vec3 pixels[512 * 512];
}
writepixel_data;


void main()
{
	out_colour = texture(tex_sampler, frag_texcoord);

	int fbo_width = 512;

	int x = int(gl_FragCoord.x * fbo_width);
	int y = int(gl_FragCoord.y * fbo_width);

	int pixel_position = int(gl_FragCoord.x) + int(gl_FragCoord.y) * fbo_width;

	uint8_t out_red = uint8_t(out_colour.r * 255);
	uint8_t out_green = uint8_t(out_colour.g * 255);
	uint8_t out_blue = uint8_t(out_colour.b * 255);

	writepixel_data.pixels[pixel_position] = u8vec3(out_red, out_green, out_blue); // write the rgb of out_colour to ssbo
	out_colour.rgb = vec3(writepixel_data.pixels[pixel_position]) / 255.0;
	// writepixel_data.pixels[pixel_position] = vec4(1.0, 1.0, 1.0, 1.0);
	// out_colour							   = writepixel_data.pixels[pixel_position];
}
