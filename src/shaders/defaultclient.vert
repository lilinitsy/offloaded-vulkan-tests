#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;


layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_colour;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec3 frag_colour;
layout(location = 1) out vec2 frag_texcoord;
layout(location = 2) out vec2 frag_quad_uv; // fullscreen quad uv coordinates

void main()
{
	//gl_Position = ubo.projection * ubo.view * ubo.model * vec4(in_position, 1.0);
	frag_colour = in_colour;
	frag_texcoord = in_texcoord;
	frag_quad_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(frag_texcoord * 2.0f - 1.0f, 0.0f, 1.0f);

}