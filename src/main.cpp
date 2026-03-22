#include "app/window.h"
#include "app/config.h"
#include "app/canvas.h"
#include "logging/logger.h"
#include "logging/spdlog_logger.h"
#include "rendering/grid_renderer.h"

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

    // ── Grid & Canvas ─────────────────────────────────────────────────────────
    Canvas canvas;
    GridRenderer grid;

    int gridCols = 29;
    int gridRows = 29;
    grid.resize(gridCols, gridRows);

    bool viewCentred = false; // centre view on first frame

    LOG_INFO("Main", "Perler Bead Simulator started");

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (!window.shouldClose()) {
        window.pollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ── Control panel ─────────────────────────────────────────────────────
        {
            ImGui::Begin("Controls");
            ImGui::Text("FPS: %.1f (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);

            ImGui::Separator();
            ImGui::Text("Grid Size");
            bool changed = false;
            changed |= ImGui::SliderInt("Columns", &gridCols, 1, 100);
            changed |= ImGui::SliderInt("Rows",    &gridRows, 1, 100);
            if (changed) {
                grid.resize(gridCols, gridRows);
            }

            if (ImGui::Button("Reset View")) {
                viewCentred = false; // will re-centre next frame
            }

            ImGui::Separator();
            ImGui::Text("Zoom: %.1f px/unit", canvas.scale());
            ImGui::Text("Offset: (%.1f, %.1f)", canvas.offset().x, canvas.offset().y);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Controls:");
            ImGui::BulletText("Scroll: Zoom in/out");
            ImGui::BulletText("Right-click drag: Pan");
            ImGui::BulletText("Middle-click drag: Pan");

            ImGui::End();
        }

        // ── Canvas (full remaining area) ──────────────────────────────────────
        {
            ImGui::SetNextWindowPos(ImVec2(280, 0), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(
                ImVec2(static_cast<float>(window.getWidth()) - 280.0f,
                       static_cast<float>(window.getHeight())),
                ImGuiCond_FirstUseEver);

            ImGui::Begin("Canvas", nullptr,
                         ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse);

            if (canvas.begin()) {
                // Centre the view on the first valid frame
                if (!viewCentred) {
                    canvas.centreView(static_cast<float>(grid.cols()),
                                      static_cast<float>(grid.rows()));
                    viewCentred = true;
                }

                grid.draw(canvas);
                canvas.end();
            }

            ImGui::End();
        }

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
