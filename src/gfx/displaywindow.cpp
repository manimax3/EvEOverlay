#include "displaywindow.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"

#include "../logging.h"

namespace {
bool initGlfw()
{
    static bool init = false;

    if (!init) {
        init = glfwInit();
        glfwSetErrorCallback([](int error, const char *description) { eo::log::error("GLFW Error {0}: {1}", error, description); });
    }

    return init;
}

void terminateGlfw()
{
    static bool terminated = false;
    if (!terminated) {
        glfwTerminate();
        terminated = true;
    }
}
}

void eo::DisplayWindow::pollEvents() { glfwPollEvents(); }

eo::DisplayWindow::DisplayWindow(int width, int height, std::string name, int posx, int posy)
{
    log::info("Creating a new window {0}", name);

    if (!initGlfw()) {
        log::error("Could not initialize glfw!");
        return; // TODO Mayebe throw an  exception here
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE);

    mWindow = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);

    if (!mWindow) {
        log::error("Could not create a valid window");
        return;
    }

    glfwSetWindowUserPointer(mWindow, this);

    glfwSetKeyCallback(mWindow, [](auto *window, auto... args) {
        static_cast<DisplayWindow *>(glfwGetWindowUserPointer(window))->keyboardInput(args...);
    });
    glfwSetCharCallback(mWindow, [](auto *window, auto... args) {
        static_cast<DisplayWindow *>(glfwGetWindowUserPointer(window))->characterInput(args...);
    });
    glfwSetCursorPosCallback(
        mWindow, [](auto *window, auto... args) { static_cast<DisplayWindow *>(glfwGetWindowUserPointer(window))->cursorInput(args...); });
    glfwSetCursorEnterCallback(mWindow, [](auto *window, auto... args) {
        static_cast<DisplayWindow *>(glfwGetWindowUserPointer(window))->cursorenterInput(args...);
    });
    glfwSetMouseButtonCallback(mWindow, [](auto *window, auto... args) {
        static_cast<DisplayWindow *>(glfwGetWindowUserPointer(window))->mousebuttonInput(args...);
    });
    glfwSetScrollCallback(
        mWindow, [](auto *window, auto... args) { static_cast<DisplayWindow *>(glfwGetWindowUserPointer(window))->scrollInput(args...); });
    glfwSetFramebufferSizeCallback(mWindow, [](auto *window, auto... args) {
        static_cast<DisplayWindow *>(glfwGetWindowUserPointer(window))->framebufferResizeCallback(args...);
    });

    glfwMakeContextCurrent(mWindow);
    gladLoadGLLoader((GLADloadproc)(glfwGetProcAddress));
}

bool eo::DisplayWindow::shouldWindowClose() const { return glfwWindowShouldClose(mWindow); }

void eo::DisplayWindow::frame()
{
    glfwMakeContextCurrent(mWindow);
    renderContents();
    glfwSwapBuffers(mWindow);
}

void eo::DisplayWindow::renderContents()
{
    glClearColor(1., .0, .0, .25);
    glClear(GL_COLOR_BUFFER_BIT);
}

void eo::DisplayWindow::framebufferResizeCallback(int width, int height)
{
    glfwMakeContextCurrent(mWindow);
    glViewport(0, 0, width, height);
    onFramebufferResize(width, height);
}

eo::math::vec2 eo::DisplayWindow::getFramebufferSize() const
{
    int x, y;
    glfwGetFramebufferSize(mWindow, &x, &y);
    return { static_cast<float>(x), static_cast<float>(y) };
}

eo::DisplayWindow::~DisplayWindow()
{
    if (mWindow) {
        glfwSetWindowUserPointer(mWindow, nullptr);
        glfwDestroyWindow(mWindow);
    }
    terminateGlfw();
}

void eo::DisplayWindow::setClipboard(const char *content) { glfwSetClipboardString(mWindow, content); }

const char *eo::DisplayWindow::getClipboard() const { return glfwGetClipboardString(mWindow); }

double eo::DisplayWindow::getTime() { return glfwGetTime(); }
