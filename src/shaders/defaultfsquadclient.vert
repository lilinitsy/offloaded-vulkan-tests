#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 out_quad_uv;
layout(location = 1) out vec2 out_server_quad_uv;

float CLIENTFOV = 45.0;
float SERVERWIDTH = 512.0;
float SERVERHEIGHT = 512.0;
float CLIENTWIDTH = 1920.0;
float CLIENTHEIGHT = 1080.0;


void main()
{
	float xscale = CLIENTWIDTH / SERVERWIDTH;
	float yscale = CLIENTHEIGHT / SERVERHEIGHT;
	out_quad_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	out_server_quad_uv = vec2(out_quad_uv.x * xscale + .5, out_quad_uv.y * yscale + .5);
	gl_Position = vec4(out_quad_uv * 2.0f - 1.0f, 0.0f, 1.0f);
}