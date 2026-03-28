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

    // Current framebuffer size (updated every frame via pollEvents)
    int getWidth()  const { return width_; }
    int getHeight() const { return height_; }

private:
    GLFWwindow* window_ = nullptr;
    int width_;
    int height_;

    // GLFW callback to track window resize
    static void framebufferSizeCallback(GLFWwindow* win, int w, int h);
};

#endif // WINDOW_H
