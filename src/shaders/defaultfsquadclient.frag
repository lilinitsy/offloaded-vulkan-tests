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

// Segment off serverframe and client frame, use these hardcoded values for now
int server_x_start = 704;
int server_x_end = 1216;
int server_y_start = 284;
int server_y_end = 796;

void main()
{
	bool bool_in_server_region = 
		gl_FragCoord.x >= server_x_start && gl_FragCoord.x <= server_x_end &&
		gl_FragCoord.y >= server_y_start && gl_FragCoord.y <= server_y_end;
	int in_server_region = int(bool_in_server_region);

	out_colour = in_server_region * texture(server_frame_sampler, quad_uv) + (1 - in_server_region) * texture(local_frame_sampler, quad_uv);
	

}