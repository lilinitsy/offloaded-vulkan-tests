#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 quad_uv;

layout(location = 0) out vec4 out_colour;

layout(binding = 0) uniform sampler2D server_frame_sampler;

void main()
{
	out_colour = texture(server_frame_sampler, quad_uv);
}