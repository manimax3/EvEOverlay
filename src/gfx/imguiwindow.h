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

    double mLastFrame = 0.0;
	math::vec2 mLastCursorPos;
};
}
