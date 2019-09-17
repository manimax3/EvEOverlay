#include "gfx/imguiwindow.h"
#include <iostream>

int main()
{
	eo::ImguiWindow window(200, 200, "Test", 0, 0);

	while(!window.shouldWindowClose()) {
		window.pollEvents();
		window.frame();
	}
	return 0;
}
