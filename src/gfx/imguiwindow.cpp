#include "imguiwindow.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "imgui.h"

namespace {
const char *vertex_shader = R"glsl(
		#version 330 core
		layout(location = 0) in vec2 Position;
		layout(location = 1) in vec2 uv;
		layout(location = 2) in vec4 color;
		
		uniform mat4 projection;

		out vec3 Frag_UV;
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
		
		in vec3 Frag_UV;
		in vec4 Frag_Color;

		uniform sampler2D texture;

		out vec4 color;

		void main()
		{
			color = Frag_Color * texture(tex, Frag_UV.st);
		}
	)glsl";
}

eo::ImguiWindow::ImguiWindow(int width, int height, std::string name, int xpos, int ypos)
    : DisplayWindow(width, height, std::move(name), xpos, ypos)
{
    mContext = ImGui::CreateContext(); // TODO Pass a shared font atlas
    ImGui::SetCurrentContext(mContext);
    ImGui::StyleColorsDark();
    auto &io = ImGui::GetIO();

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

    {
        uint32 *pixels;
        int     width, height;
        io.Fonts->GetTexDataAsRGBA32(reinterpret_cast<byte **>(&pixels), &width, &height);

        glGenTextures(1, &mFontTexture);
        glBindTexture(GL_TEXTURE_2D, mFontTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32I, width, height, 0, GL_RGBA, GL_UNSIGNED_INT, pixels);
        io.Fonts->TexID = reinterpret_cast<void *>(mFontTexture);
    }

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
    }

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

eo::ImguiWindow::~ImguiWindow()
{
    if (mContext) {
        ImGui::DestroyContext(mContext);
    }
}
