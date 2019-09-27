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

#include "imguiwindow.h"
#include "logging.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "IconsForkAwesome.h"
#include "glad/glad.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"

namespace {
constexpr float font_size = 32.f;

const char *vertex_shader = R"glsl(
#version 330 core
layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

uniform mat4 projection;

out vec2 Frag_UV;
out vec4 Frag_Color;

void main()
{
	Frag_UV = uv;
	Frag_Color = color;
	gl_Position = projection * vec4(Position.xy, 0, 1);
}
	)glsl";

const char *fragment_shader = R"glsl(
#version 330 core

in vec2 Frag_UV;
in vec4 Frag_Color;

uniform sampler2D tex;

out vec4 color;

void main()
{
	color = Frag_Color * texture(tex, Frag_UV.st);
}
	)glsl";

void glcheck(int line)
{
    GLenum error = glGetError();
    if (error == GL_NO_ERROR)
        return;
    while (error != GL_NO_ERROR) {

        eo::log::error("Opengl error: {0} before line {1}", error, line);

        error = glGetError();
    }
}
}
#define GLCHECK glcheck(__LINE__);

eo::ImguiWindow::ImguiWindow(int width, int height, std::string name, int xpos, int ypos)
    : DisplayWindow(width, height, std::move(name), xpos, ypos)
{
    mContext = ImGui::CreateContext(); // TODO Pass a shared font atlas
    ImGui::SetCurrentContext(mContext);
    ImGui::StyleColorsDark();
    auto &io = ImGui::GetIO();

    ImGui::GetStyle().WindowRounding = 0.f;

    if (!io.Fonts->AddFontFromFileTTF("assets/NotoMono-Regular.ttf", font_size)) {
        throw std::logic_error("Could not load/find font file: NotoMono-Regular.ttf");
    }

    const ImWchar icons_ranges[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
    ImFontConfig  icons_config;
    icons_config.PixelSnapH = true;
    icons_config.MergeMode  = true;

    if (!io.Fonts->AddFontFromFileTTF("assets/" FONT_ICON_FILE_NAME_FK, font_size, &icons_config, icons_ranges)) {
        throw std::logic_error("Could not load/find font file: ForkAwesome");
    }

    io.Fonts->Build();

    const auto display_size    = getFramebufferSize();
    io.DisplaySize.x           = display_size.x;
    io.DisplaySize.y           = display_size.y;
    io.DisplayFramebufferScale = { 1, 1 };

    glGenBuffers(1, &mVbo);
    glGenBuffers(1, &mIbo);
    glGenVertexArrays(1, &mVao);

    glBindVertexArray(mVao);
    glBindBuffer(GL_ARRAY_BUFFER, mVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)offsetof(ImDrawVert, pos));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)offsetof(ImDrawVert, uv));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid *)offsetof(ImDrawVert, col));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    GLCHECK

    {
        uint32 *pixels;
        int     width, height;
        io.Fonts->GetTexDataAsRGBA32(reinterpret_cast<byte **>(&pixels), &width, &height);

        glGenTextures(1, &mFontTexture);
        glBindTexture(GL_TEXTURE_2D, mFontTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        io.Fonts->TexID = reinterpret_cast<void *>(mFontTexture);
    }
    GLCHECK

    {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertex_shader, NULL);
        glCompileShader(vertexShader);

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragment_shader, NULL);
        glCompileShader(fragmentShader);

        mShaderProgram = glCreateProgram();
        glAttachShader(mShaderProgram, vertexShader);
        glAttachShader(mShaderProgram, fragmentShader);
        glLinkProgram(mShaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    GLCHECK

    io.KeyMap[ImGuiKey_Tab]        = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow]  = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow]    = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow]  = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp]     = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown]   = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home]       = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End]        = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert]     = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete]     = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace]  = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space]      = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter]      = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape]     = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A]          = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C]          = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V]          = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X]          = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y]          = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z]          = GLFW_KEY_Z;

    io.SetClipboardTextFn = [](void *ud, const char *content) {
        auto *imguiwindow = static_cast<ImguiWindow *>(ud);
        imguiwindow->setClipboard(content);
    };

    io.GetClipboardTextFn = [](void *ud) {
        auto *imguiwindow = static_cast<ImguiWindow *>(ud);
        return imguiwindow->getClipboard();
    };

    io.ClipboardUserData = this;
}

void eo::ImguiWindow::renderContents()
{
    glClearColor(0., 0., 0., 0.);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::SetCurrentContext(mContext);
    auto &io = ImGui::GetIO();

    const auto display_size = getFramebufferSize();
    io.DisplaySize.x        = display_size.x;
    io.DisplaySize.y        = display_size.y;
    io.DeltaTime            = getTime() - mLastFrame;

    // TODO Might be wanky either: glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
    // or use the callbacks
    io.MouseDown[0] = glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    io.MouseDown[1] = glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    io.MouseDown[2] = glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(display_size.x, display_size.y));
    ImGui::SetNextWindowPos({ 0.f, 0.f });
    ImGui::Begin(mName.c_str(), NULL, ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoScrollbar);
    renderImguiContents();
    ImGui::End();
    ImGui::EndFrame();
    ImGui::Render();

    auto       draw_data  = ImGui::GetDrawData();
    const auto projection = math::ortho(0.f, display_size.x, display_size.y, 0.f, 1.f, -1.f);

    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(mShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(mShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform1i(glGetUniformLocation(mShaderProgram, "tex"), 0);
    glBindSampler(0, 0);
    glBindTexture(GL_TEXTURE_2D, mFontTexture);

    glBindVertexArray(mVao);
    glBindBuffer(GL_ARRAY_BUFFER, mVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);

    GLCHECK

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList *cmd_list          = draw_data->CmdLists[n];
        const ImDrawIdx * idx_buffer_offset = nullptr;

        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid *)cmd_list->VtxBuffer.Data,
                     GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
                     (const GLvoid *)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                glScissor((int)pcmd->ClipRect.x, (int)(display_size.y - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                          (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                               idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glBindVertexArray(0);

    GLCHECK

    mLastFrame = getTime();
}

void eo::ImguiWindow::keyboardInput(int key, int scancode, int action, int mods)
{
    if (mods & GLFW_MOD_CONTROL && key == GLFW_KEY_B && action == GLFW_PRESS) {
        // FIXME Doesnt work see: https://github.com/glfw/glfw/issues/1566
        glfwSetWindowAttrib(mWindow, GLFW_DECORATED, glfwGetWindowAttrib(mWindow, GLFW_DECORATED) == GLFW_TRUE ? GLFW_FALSE : GLFW_TRUE);
        return;
    }

    ImGui::SetCurrentContext(mContext);
    auto &io = ImGui::GetIO();
    if (action == GLFW_PRESS) {
        io.KeysDown[key] = true;
    } else if (action == GLFW_RELEASE) {
        io.KeysDown[key] = false;
    }

    io.KeyAlt   = mods & GLFW_MOD_ALT;
    io.KeyCtrl  = mods & GLFW_MOD_CONTROL;
    io.KeyShift = mods & GLFW_MOD_SHIFT;
    io.KeySuper = mods & GLFW_MOD_SUPER;
}

void eo::ImguiWindow::characterInput(uint codepoint)
{
    ImGui::SetCurrentContext(mContext);
    auto &io = ImGui::GetIO();
    if (codepoint > 0 && codepoint < 0x10000) {
        io.AddInputCharacter(static_cast<unsigned short>(codepoint));
    }
}

void eo::ImguiWindow::cursorInput(double xpos, double ypos)
{
    ImGui::SetCurrentContext(mContext);
    auto &io    = ImGui::GetIO();
    io.MousePos = { static_cast<float>(xpos), static_cast<float>(ypos) };
}

void eo::ImguiWindow::scrollInput(double xoffset, double yoffset)
{
    ImGui::SetCurrentContext(mContext);
    auto &io = ImGui::GetIO();
    io.MouseWheelH += static_cast<float>(xoffset);
    io.MouseWheel += static_cast<float>(yoffset);
}

eo::ImguiWindow::~ImguiWindow()
{
    ImGui::SetCurrentContext(mContext);
    if (mContext) {
        ImGui::DestroyContext(mContext);
    }

    glfwMakeContextCurrent(mWindow);
    glDeleteTextures(1, &mFontTexture);
    glDeleteBuffers(1, &mVbo);
    glDeleteBuffers(1, &mIbo);
    glDeleteVertexArrays(1, &mVao);
    glDeleteProgram(mShaderProgram);

    GLCHECK
}

void eo::ImguiWindow::renderImguiContents()
{
    for (int i = 0; i < 12; ++i) {
        const std::string is = std::to_string(i);
        if (ImGui::CollapsingHeader(is.c_str())) {
            for (int j = 0; j < 50; j++) {
                ImGui::Button("Drueck mich");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", fmt::format("Hallo {0} Was geht", ICON_FK_BICYCLE).c_str());
            }
        }
    }
}
