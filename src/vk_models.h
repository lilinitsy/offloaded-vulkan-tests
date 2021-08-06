#ifndef VK_MODELS_H
#define VK_MODELS_H

#include <unordered_map>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "vertex.h"

struct Model
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	glm::vec3 position; // center position

	Model()
	{
		// do nothing
	}

	Model(std::string model_path, std::string texture_path, glm::vec3 pos)
	{
		load_model(model_path, texture_path);
		position = pos;

		for(uint32_t i = 0; i < vertices.size(); i++)
		{
			vertices[i].position += position;
		}
	}


	void load_model(std::string model_path, std::string texture_path)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warnings;
		std::string errors;

		if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warnings, &errors, model_path.c_str()))
		{
			throw std::runtime_error(warnings + errors);
		}

		std::unordered_map<Vertex, uint32_t> unique_vertices;

		for(uint32_t i = 0; i < shapes.size(); i++)
		{
			for(uint32_t j = 0; j < shapes[i].mesh.indices.size(); j++)
			{
				Vertex vertex;
				vertex.position = {
					attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index],
					attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 1],
					attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 2],
				}; // * by 3 since the vertices are in an array of floats
				vertex.colour	= glm::vec3(1.0f, 1.0f, 1.0f);
				vertex.texcoord = {
					attrib.texcoords[2 * shapes[i].mesh.indices[j].texcoord_index],
					1.0f - attrib.texcoords[2 * shapes[i].mesh.indices[j].texcoord_index + 1],
				}; // * by 2 since texcoords are in an array of floats

				if(unique_vertices.count(vertex) == 0)
				{
					unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(unique_vertices[vertex]);
			}
		}
	}
};




#endif