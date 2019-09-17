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
    virtual void renderImguiContents() {}

private:
    ImGuiContext *mContext = nullptr;
    uint          mVbo, mIbo, mVao, mFontTexture;
    uint          mShaderProgram;
};
}
