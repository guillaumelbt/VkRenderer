extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

#include "app/Application.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>


int main()
{
	Application app;

	try
	{
		app.Run();
	}
	catch(const std::exception &e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}