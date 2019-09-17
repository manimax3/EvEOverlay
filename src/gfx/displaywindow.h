#pragma once
#include <string>

struct GLFWwindow;

namespace eo {

/*
 * DisplayWindow:
 *  - Creates a window at the specified position and size
 *  - Instatntiates an opengl context
 *  - Sets up the imgui rendering
 *  - Provides signals for handling the contents of the window
 */

class DisplayWindow {
public:
    static void pollEvents();

public:
    explicit DisplayWindow(int width, int height, std::string name, int posx, int posy);
    ~DisplayWindow();

    bool shouldWindowClose() const;

    void frame();

private:
    GLFWwindow *mWindow = nullptr;
};
}
