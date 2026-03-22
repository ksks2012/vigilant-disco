#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>
#include <string>

// GLFW window wrapper with OpenGL 3.3 core context.
// Uses the ImGui bundled OpenGL loader (no GLEW needed).
class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    // Non-copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();

    GLFWwindow* getHandle() const { return window_; }
    int getWidth()  const { return width_; }
    int getHeight() const { return height_; }

private:
    GLFWwindow* window_ = nullptr;
    int width_;
    int height_;
};

#endif // WINDOW_H
