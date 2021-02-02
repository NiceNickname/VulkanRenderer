#pragma once
#include "GLFW/glfw3.h"
class Window
{
public:
	static Window* CreateWindow();
	Window();

	void shutDown();
	static void closeCallback(GLFWwindow* window);
	bool shouldClose();
	void update();
	inline GLFWwindow* getPointer() { return window; }


private:
	


private:
	GLFWwindow* window;
	uint32_t width = 1280, height = 720;
	bool close = false;
};