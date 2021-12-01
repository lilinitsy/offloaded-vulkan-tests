#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 frag_colour;
layout(location = 1) in vec2 frag_texcoord;

layout(location = 0) out vec4 out_colour;

layout(binding = 1) uniform sampler2D tex_sampler;

layout(std140, binding = 2) writeonly buffer WritePixelData
{
	vec3 pixels[];
} writepixel_data;


void main()
{
	out_colour = texture(tex_sampler, frag_texcoord);

	int fbo_width = 512;

	int x = int(gl_FragCoord.x * fbo_width);
	int y = int(gl_FragCoord.y * fbo_width);

	int pixel_position = x + y * fbo_width;
	writepixel_data.pixels[pixel_position] = out_colour.rgb; // write the rgb of out_colour to ssbo
}