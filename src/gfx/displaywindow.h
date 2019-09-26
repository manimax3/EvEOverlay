// Copyright 2019 Maximilian Schiller
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <string>

#include "../math.h"
#include "../util.h"

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
    static void   pollEvents();
    static double getTime();

public:
    explicit DisplayWindow(int width, int height, std::string name, int posx, int posy);
    virtual ~DisplayWindow();

    bool shouldWindowClose() const;

    void frame();

    math::vec2  getFramebufferSize() const;
    void        setClipboard(const char *content);
    const char *getClipboard() const;

protected:
    virtual void renderContents();

    virtual void keyboardInput(int key, int scancode, int action, int mods) {}
    virtual void characterInput(unsigned int codepoint) {}
    virtual void cursorInput(double xpos, double ypos) {}
    virtual void cursorenterInput(int entered) {}
    virtual void mousebuttonInput(int button, int action, int mods) {}
    virtual void scrollInput(double xoffset, double yoffset) {}

    virtual void onFramebufferResize(int widht, int height) {}
    void         framebufferResizeCallback(int width, int height);

    GLFWwindow *mWindow = nullptr;
	std::string mName;
};
}
