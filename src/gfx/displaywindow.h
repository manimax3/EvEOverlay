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
    virtual ~DisplayWindow();

    bool shouldWindowClose() const;

    void frame();

protected:
	virtual void renderContents();

    virtual void keyboardInput(int key, int scancode, int action, int mods) {}
    virtual void characterInput(unsigned int codepoint) {}
    virtual void cursorInput(double xpos, double ypos) {}
    virtual void cursorenterInput(int entered) {}
    virtual void mousebuttonInput(int button, int action, int mods) {}
    virtual void scrollInput(double xoffset, double yoffset) {}

private:
    GLFWwindow *mWindow = nullptr;
};
}
