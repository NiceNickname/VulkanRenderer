#include "Renderer/Renderer.h"
#include "Window.h"
#include <memory>

struct GLFWwindow;

class App
{
public:
	void run();

private:
	void start();
	void loop();
	void shutDown();

private:
	std::unique_ptr<Window> window;
};