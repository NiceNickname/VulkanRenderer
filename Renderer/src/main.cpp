#include "core/Application.h"
#include <stdexcept>
#include <iostream>

void main()
{
	App app;

	try
	{
		app.run();
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}