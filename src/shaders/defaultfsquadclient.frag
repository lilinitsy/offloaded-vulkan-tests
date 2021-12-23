#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 quad_uv;
layout(location = 1) in vec2 server_quad_uv;

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

float CLIENTFOV = 45.0;
float SERVERWIDTH = 512.0;
float CLIENTWIDTH = 1920.0;


void main()
{
	//vec2 server_quaduv = quad_uv * (CLIENTFOV * (SERVERWIDTH / CLIENTWIDTH));
	int xmax = (1920 / 2 + (512 / 2));
	int xmin = (1920 / 2 - (512 / 2));
	int ymax = (1080 / 2 + (512 / 2));
	int ymin = (1080 / 2 - (512 / 2));

	bool valid_server_pixel = 	gl_FragCoord.x <= xmax && gl_FragCoord.x >= xmin &&
								gl_FragCoord.y <= ymax && gl_FragCoord.y >= ymin;
	out_colour = int(valid_server_pixel) * texture(server_frame_sampler, server_quad_uv) + texture(local_frame_sampler, quad_uv);
}