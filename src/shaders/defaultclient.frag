#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 frag_colour;
layout(location = 1) in vec2 frag_texcoord;
layout(location = 2) in vec2 frag_quad_uv;

layout(location = 0) out vec4 out_colour;

layout(binding = 1) uniform sampler2D tex_sampler;


void main()
{
	out_colour = texture(tex_sampler, frag_quad_uv);
}