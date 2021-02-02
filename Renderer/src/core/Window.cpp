#include "GLFW/glfw3.h"
#include "Window.h"
#include <stdexcept>


Window::Window()
{
	if (!glfwInit())
		throw std::runtime_error("cannot create window!");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, "Vulkan renderer", NULL, NULL);

	glfwSetWindowUserPointer(window, this);
	glfwSetWindowCloseCallback(window, closeCallback);

}

Window* Window::CreateWindow()
{
	Window* window = new Window();
	return window;
}

void Window::shutDown()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Window::closeCallback(GLFWwindow* window)
{
	auto windowInstance = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	windowInstance->close = true;
}

bool Window::shouldClose()
{
	return close;
}

void Window::update()
{
	glfwPollEvents();
}



