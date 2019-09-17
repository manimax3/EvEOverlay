#include "gfx/displaywindow.h"
#include <iostream>

int main()
{
	eo::DisplayWindow window(200, 200, "Test", 0, 0);

	while(!window.shouldWindowClose()) {
		window.pollEvents();
		window.frame();
	}
	return 0;
}
