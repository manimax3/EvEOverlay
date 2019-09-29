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
#include "displaywindow.h"

struct ImGuiContext;

namespace eo {
class ImguiWindow : public DisplayWindow {
public:
    explicit ImguiWindow(int width, int height, std::string name, int xpos, int ypos);
    ~ImguiWindow();

protected:
    void         renderContents() override;
    virtual void renderImguiContents();

    void keyboardInput(int key, int scancode, int action, int mods) override;
    void characterInput(unsigned int codepoint) override;
    void cursorInput(double xpos, double ypos) override;
    void scrollInput(double xoffset, double yoffset) override;

private:
    ImGuiContext *mContext = nullptr;
    uint          mVbo, mIbo, mVao, mFontTexture;
    uint          mShaderProgram;

    double     mLastFrame = 0.0;
};
}
