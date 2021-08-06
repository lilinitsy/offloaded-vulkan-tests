#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 quad_uv;

layout(location = 0) out vec4 out_colour;

layout(binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 projection;

	float x;
	float y;
	float width;
	float height;
} ubo;
layout(binding = 1) uniform sampler2D server_frame_sampler;
layout(binding = 2) uniform sampler2D local_frame_sampler;

void main()
{
	out_colour = texture(server_frame_sampler, quad_uv) + texture(local_frame_sampler, quad_uv);
}