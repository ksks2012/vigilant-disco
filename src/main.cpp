#include "app/window.h"
#include "app/config.h"
#include "app/editor_state.h"
#include "app/control_panel.h"
#include "app/canvas_panel.h"
#include "logging/logger.h"
#include "logging/spdlog_logger.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <memory>
#include <string>

int main(int /*argc*/, char* /*argv*/[]) {
    // ── Logger setup ──────────────────────────────────────────────────────────
    auto logger = std::make_shared<SpdlogLogger>();
    GlobalLogger::set(logger);

    // ── Config ────────────────────────────────────────────────────────────────
    Config config;
    try {
        config.loadFromFile("etc/config.json");
    } catch (const ConfigError& e) {
        LOG_ERROR("Main", std::string("Config error: ") + e.what());
        return 1;
    }

    LogLevel logLevel = static_cast<LogLevel>(config.logger_level);
    logger->set_level(logLevel);

    // ── Window + OpenGL context ───────────────────────────────────────────────
    Window window(config.window.title, config.window.width, config.window.height);

    // ── ImGui setup ───────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window.getHandle(), true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Background clear colour (soft light grey)
    ImVec4 clear_color = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);

    // ── Editor state & UI panels ──────────────────────────────────────────────
    EditorState  state;
    ControlPanel controlPanel;
    CanvasPanel  canvasPanel;

    state.palette.loadFromFile("etc/palette.json");
    state.beadGrid.resize(state.gridCols, state.gridRows);

    LOG_INFO("Main", "Perler Bead Simulator started");

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (!window.shouldClose()) {
        window.pollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ── UI panels ─────────────────────────────────────────────────────────
        controlPanel.draw(state);
        canvasPanel.draw(state, window);

        // ── Render ────────────────────────────────────────────────────────────
        ImGui::Render();
        glViewport(0, 0, window.getWidth(), window.getHeight());
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.swapBuffers();
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    LOG_INFO("Main", "Shutdown complete");
    return 0;
}
