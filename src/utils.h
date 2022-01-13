#ifndef UTILS_H
#define UTILS_H

#include <cstring>
#include <fstream>

#include <vector>

static std::vector<char> parse_shader_file(const std::string &filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if(!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}


void rgba_to_rgb(const uint8_t *__restrict__ in, uint8_t *__restrict__ out, size_t len)
{
	for(size_t i = 0, j = 0; i < len; i += 4, j += 3)
	{
		out[j + 0] = in[i + 0];
		out[j + 1] = in[i + 1];
		out[j + 2] = in[i + 2];
	}
}


void rgb_to_rgba(const uint8_t *__restrict__ in, uint8_t *__restrict__ out, size_t len)
{
	for(size_t i = 0, j = 0; i < len; i += 4, j += 3)
	{
		out[i + 0] = in[j + 0];
		out[i + 1] = in[j + 1];
		out[i + 2] = in[j + 2];
		out[i + 3] = 255;
	}
}


#endif