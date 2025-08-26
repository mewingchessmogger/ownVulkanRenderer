#include <pch.h>
#include <engine.h>
#include "vkutils.h"
#include <Vulkancontext.h>
#include <BufferContext.h>
#include <WindowContext.h>
void majorHandler(Camera& camera, WindowContext* wtx);
void mouseHandler(Camera& camera, float dt);
void inputHandler(Camera& camera, float dt, WindowContext* wtx);

void Engine::HandlerIO(Camera &camera) {
	majorHandler(camera, wtx);
}
void majorHandler(Camera& camera, WindowContext* wtx) {
	static float lastTime = glfwGetTime();

	float currTime = glfwGetTime();

	float dT = currTime - lastTime;

	lastTime = currTime;

	static bool gWasPressed = false;
	static bool toggleCursor = false;

	glfwGetCursorPos(wtx->window, &camera.newPos.x, &camera.newPos.y);
	if (glfwGetKey(wtx->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(wtx->window, GLFW_TRUE);
	}
	int state = glfwGetKey(wtx->window, GLFW_KEY_G);

	if (state == GLFW_PRESS && !gWasPressed) {
		toggleCursor = !toggleCursor;

		if (!toggleCursor) {
			glfwSetInputMode(wtx->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			double cx, cy;
			glfwGetCursorPos(wtx->window, &cx, &cy);
			camera.oldPos = { cx, cy };
		}
		else {
			glfwSetInputMode(wtx->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		}

	}


	if (!toggleCursor) {
		inputHandler(camera, dT, wtx);
		mouseHandler(camera, dT);
	}

	gWasPressed = (state == GLFW_PRESS);

}
void mouseHandler(Camera& c, float dT) {


	float xoffset = c.newPos.x - c.oldPos.x;
	float yoffset = c.newPos.y - c.oldPos.y;

	float sens = 10.0f * dT;


	c.yaw += sens * xoffset;
	c.pitch += sens * yoffset;

	c.pitch = std::clamp(c.pitch, -89.0f, 89.0f);

	c.dir.x = cos(glm::radians(c.yaw)) * cos(glm::radians(c.pitch));
	c.dir.z = sin(glm::radians(c.yaw)) * cos(glm::radians(c.pitch));
	c.dir.y = sin(glm::radians(c.pitch));
	c.dir = glm::normalize(c.dir);

	c.oldPos = c.newPos;



}


void inputHandler(Camera& camera, float dT, WindowContext* wtx) {

	const float cameraSpeed = 1.0f * dT; // 

	if (glfwGetKey(wtx->window, GLFW_KEY_E) == GLFW_PRESS) {//forwrad
		camera.dir.y = 0.0f;
		camera.eye += cameraSpeed * camera.dir;

	}
	if (glfwGetKey(wtx->window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {//BACK!!!!
		camera.dir.y = 0.0f;
		camera.eye -= cameraSpeed * camera.dir;
	}
	if (glfwGetKey(wtx->window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
		camera.eye -= glm::normalize(glm::cross(camera.dir, camera.up)) * cameraSpeed;//left
	if (glfwGetKey(wtx->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera.eye += glm::normalize(glm::cross(camera.dir, camera.up)) * cameraSpeed;//right

	if (glfwGetKey(wtx->window, GLFW_KEY_LEFT) == GLFW_PRESS)
		camera.eye.y -= cameraSpeed;
	if (glfwGetKey(wtx->window, GLFW_KEY_END) == GLFW_PRESS)
		camera.eye.y += cameraSpeed;



}
