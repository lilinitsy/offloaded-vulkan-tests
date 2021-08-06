#ifndef CAMERA_H
#define CAMERA_H


#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <GLFW/glfw3.h>



struct Camera
{
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	float speed;

	Camera()
	{
		position = glm::vec3(0.0f, 0.0f, 0.0f);
		front	 = glm::vec3(0.0f, 0.0f, 1.0f);
		up		 = glm::vec3(0.0f, 1.0f, 0.0f);
		right	 = glm::cross(front * -1.0f, up);
	}

	Camera(glm::vec3 start_position)
	{
		position = start_position;
		front	 = glm::vec3(1.0f, 0.0f, 0.0f);
		up		 = glm::vec3(0.0f, 1.0f, 0.0f);
		right	 = glm::cross(up * -1.0f, up);
	}

	void move(float dt, GLFWwindow *window)
	{
		speed = dt * 0.001f;
		if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			position = glm::vec3(position.x + speed * front.x,
										   position.y + speed * front.y,
										   position.z + speed * front.z);
		}

		if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			position = glm::vec3(position.x - speed * front.x,
										   position.y - speed * front.y,
										   position.z - speed * front.z);
		}

		if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		{
			position.y += speed * 0.4f;
		}

		if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		{
			position.y -= speed * 0.4f;
		}

		if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			position -= glm::normalize(glm::cross(front, up)) * speed;
		}

		if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			position += glm::normalize(glm::cross(front, up)) * speed;
		}
	}
};


#endif