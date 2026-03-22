#include "app/window.h"
#include "logging/logger.h"
#include <stdexcept>
#include <string>

static void glfwErrorCallback(int error, const char* description) {
    LOG_ERROR("GLFW", "Error " + std::to_string(error) + ": " + description);
}

Window::Window(const std::string& title, int width, int height)
    : width_(width), height_(height)
{
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialise GLFW");
    }

    // Request OpenGL 3.3 core (sufficient for ImGui + 2D rendering)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(width_, height_, title.c_str(), nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // vsync

    LOG_INFO("Window", "Created " + std::to_string(width_) + "x" +
             std::to_string(height_) + " window: " + title);
}

Window::~Window() {
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void Window::swapBuffers() {
    glfwSwapBuffers(window_);
}

void Window::pollEvents() {
    glfwPollEvents();
}
