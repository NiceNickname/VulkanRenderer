#include "Application.h"
#include "Renderer/Renderer.h"
#include <memory>

void App::run()
{
	start();
	loop();
	shutDown();
}

void App::start()
{
	window = std::unique_ptr<Window>(Window::CreateWindow());
	Renderer::init(window->getPointer());
}

void App::loop()
{
	while (!window->shouldClose())
	{
		window->update();
		Renderer::draw();
	}
}

void App::shutDown()
{
	Renderer::shutdown();
	window->shutDown();
}

